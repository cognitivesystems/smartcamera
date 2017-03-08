#include <glipf/processors/foreground-histogram-processor.h>

#include <glipf/gles-utils/shader-builder.h>
#include <glipf/gles-utils/glsl-program-builder.h>

#include <boost/variant/get.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstring>


#define BASE_TEXTURE_WIDTH 160
#define BASE_TEXTURE_HEIGHT 120
#define HISTOGRAM_TEXTURE_WIDTH 10
#define HISTOGRAM_TEXTURE_HEIGHT 10
#define HISTOGRAM_TEXTURE_AREA (HISTOGRAM_TEXTURE_WIDTH * HISTOGRAM_TEXTURE_HEIGHT)
#define MODEL_GRID_WIDTH 4
#define MODEL_GRID_HEIGHT 4
#define MODEL_GRID_AREA (MODEL_GRID_WIDTH * MODEL_GRID_HEIGHT)
#define MODELS_PER_GRID_CELL 2
#define MODEL_GRID_MODEL_COUNT (MODEL_GRID_AREA * MODELS_PER_GRID_CELL)
#define HALF_BASE_TEXEL_WIDTH (0.5f / (MODEL_GRID_WIDTH * BASE_TEXTURE_WIDTH))
#define HALF_BASE_TEXEL_HEIGHT (0.5f / (MODEL_GRID_HEIGHT * BASE_TEXTURE_HEIGHT))


using std::pair;
using std::string;
using std::tuple;
using std::vector;


namespace glipf {
namespace processors {


enum VertexAttributeLocations : GLuint {
  kPosition = 0,
  kColor = 1,
  kCellOffset = 2
};


ForegroundHistogramProcessor::ForegroundHistogramProcessor(const sources::FrameProperties& frameProperties,
                                                           size_t maxModelCount,
                                                           const glm::mat4& mvpMatrix)
  : GlesProcessor(frameProperties)
  , mModelCount(0)
  , mMaxModelCount(maxModelCount)
  , mModelVertexBuffer(0)
  , mModelIndexBuffer(0)
  , mScatterPointsBuffer(0)
  , mHistogramGlslProgram(0)
{
  setupReductionGlslPrograms(mvpMatrix);
  setupFbos(maxModelCount);

  mHistogramGlslProgram = gles_utils::GlslProgramBuilder()
    .attachShader(gles_utils::ShaderBuilder(GL_VERTEX_SHADER)
                    .appendSourceFile("glsl/histogram-scatter.vert")
                    .compile())
    .attachShader(gles_utils::ShaderBuilder(GL_FRAGMENT_SHADER)
                    .appendSourceFile("glsl/unit-value.frag")
                    .compile())
    .bindAttribLocation(VertexAttributeLocations::kPosition, "vertex")
    .link();

  glUseProgram(mHistogramGlslProgram);
  glUniform1i(glGetUniformLocation(mHistogramGlslProgram, "tex"), 2);
  glUniform3i(glGetUniformLocation(mHistogramGlslProgram, "gridDimensions"),
              MODEL_GRID_WIDTH, MODEL_GRID_HEIGHT, MODELS_PER_GRID_CELL);

  glGenBuffers(1, &mModelVertexBuffer);
  glGenBuffers(1, &mModelIndexBuffer);
  glGenBuffers(1, &mScatterPointsBuffer);
  assertNoGlError();

  for (size_t i = 0; i < maxModelCount; ++i)
    mResultSet[std::to_string(i)] = vector<float>(HISTOGRAM_TEXTURE_AREA);

  mResultSet["total_pixel_counts"] = vector<float>(maxModelCount);
  mResultSet["histogram_coverage"] = vector<float>(maxModelCount);
}


void ForegroundHistogramProcessor::setModels(const vector<ModelData>& models,
                                             const glm::mat4& mvpMatrix)
{
  mReductionFboSets.clear();
  mHistogramFboSpecs.clear();

  mModelCount = models.size();
  assert(mModelCount <= mMaxModelCount);

  setupModelGeometry(models);
  setupHistogramBuffers(models, mvpMatrix);

  mModelAreas = computeModelAreas(models, mvpMatrix, BASE_TEXTURE_WIDTH,
                                  BASE_TEXTURE_HEIGHT);
}


ForegroundHistogramProcessor::~ForegroundHistogramProcessor() {
  glDeleteProgram(mHistogramGlslProgram);
  glDeleteBuffers(1, &mModelVertexBuffer);
  glDeleteBuffers(1, &mModelIndexBuffer);
  glDeleteBuffers(1, &mScatterPointsBuffer);

  glDeleteProgram(std::get<0>(mReductionFboSpecs[0]));

  glDeleteFramebuffers(mHistogramFbos.size(), mHistogramFbos.data());
  glDeleteTextures(mHistogramTextures.size(), mHistogramTextures.data());
  glDeleteFramebuffers(mForegroundFbos.size(), mForegroundFbos.data());
  glDeleteTextures(mForegroundTextures.size(), mForegroundTextures.data());
}


void ForegroundHistogramProcessor::setupReductionGlslPrograms(const glm::mat4& mvpMatrix) {
  GLuint mainGlslProgram = gles_utils::GlslProgramBuilder()
    .attachShader(gles_utils::ShaderBuilder(GL_VERTEX_SHADER)
                    .appendSourceFile("glsl/transformation.vert")
                    .compile())
    .attachShader(gles_utils::ShaderBuilder(GL_FRAGMENT_SHADER)
                    .appendSourceFile("glsl/include/color-space.frag")
                    .appendSourceFile("glsl/histogram-foreground.frag")
                    .compile())
    .bindAttribLocation(VertexAttributeLocations::kPosition, "vertex")
    .bindAttribLocation(VertexAttributeLocations::kColor, "vertexColor")
    .bindAttribLocation(VertexAttributeLocations::kCellOffset, "cellOffset")
    .link();

  glUseProgram(mainGlslProgram);
  glUniform2f(glGetUniformLocation(mainGlslProgram, "viewportDimensions"),
              mFrameProperties.dimensions().first,
              mFrameProperties.dimensions().second);
  glUniformMatrix4fv(glGetUniformLocation(mainGlslProgram, "projectionMatrix"),
                     1, GL_FALSE, glm::value_ptr(mvpMatrix));
  glUniform1i(glGetUniformLocation(mainGlslProgram, "tex"), 0);
  assertNoGlError();

  mReductionFboSpecs.push_back(std::make_tuple(mainGlslProgram,
                                               BASE_TEXTURE_WIDTH,
                                               BASE_TEXTURE_HEIGHT));
}


void ForegroundHistogramProcessor::addModelForegroundFbo() {
  // Prepare a texture to store the foreground of the model
  GLuint averageTexture;
  glActiveTexture(GL_TEXTURE3);
  glGenTextures(1, &averageTexture);
  glBindTexture(GL_TEXTURE_2D, averageTexture);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
               MODEL_GRID_WIDTH * BASE_TEXTURE_WIDTH,
               MODEL_GRID_HEIGHT * BASE_TEXTURE_HEIGHT, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, 0);
  assertNoGlError();

  // Prepare an FBO to store the foreground of the model
  GLuint averageFbo;
  glGenFramebuffers(1, &averageFbo);
  glBindFramebuffer(GL_FRAMEBUFFER, averageFbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_2D, averageTexture, 0);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  assertNoGlError();

  mForegroundTextures.push_back(averageTexture);
  mForegroundFbos.push_back(averageFbo);
}


void ForegroundHistogramProcessor::addHistogramFbo() {
  // Prepare a texture to store the foreground histogram of the model
  GLuint histogramTexture;
  glActiveTexture(GL_TEXTURE3);
  glGenTextures(1, &histogramTexture);
  glBindTexture(GL_TEXTURE_2D, histogramTexture);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
               MODELS_PER_GRID_CELL * MODEL_GRID_WIDTH * HISTOGRAM_TEXTURE_WIDTH,
               MODEL_GRID_HEIGHT * HISTOGRAM_TEXTURE_HEIGHT, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, 0);
  assertNoGlError();

  // Prepare an FBO to store the foreground histogram of the model
  GLuint histogramFbo;
  glGenFramebuffers(1, &histogramFbo);
  glBindFramebuffer(GL_FRAMEBUFFER, histogramFbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_2D, histogramTexture, 0);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  assertNoGlError();

  mHistogramTextures.push_back(histogramTexture);
  mHistogramFbos.push_back(histogramFbo);
}


void ForegroundHistogramProcessor::setupFbos(size_t modelCount) {
  uint_fast8_t fboCount = modelCount / MODEL_GRID_MODEL_COUNT;

  if (modelCount % MODEL_GRID_MODEL_COUNT > 0)
    ++fboCount;

  for (size_t i = 0; i < fboCount; ++i) {
    addModelForegroundFbo();
    addHistogramFbo();
  }
}


void ForegroundHistogramProcessor::setupHistogramBuffers(const std::vector<ModelData>& models,
                                                         const glm::mat4& mvpMatrix)
{
  size_t pointOffset = 0, fboScatterPointCount = 0;
  vector<tuple<uint_fast16_t, uint_fast16_t, uint_fast16_t, uint_fast16_t>> bboxVertices;
  size_t totalScatterPointCount = 0;
  uint_fast16_t modelNumber = 0;

  for (auto& model : models) {
    GLfloat xMin = mFrameProperties.dimensions().first, xMax = 0.0f;
    GLfloat yMin = mFrameProperties.dimensions().second, yMax = 0.0f;

    for (size_t i = 0; i < model.first.size(); i += 3) {
      glm::vec4 vertexVector(model.first[i], model.first[i + 1],
                             model.first[i + 2], 1.0);
      glm::vec4 projectedPosition = mvpMatrix * vertexVector;
      glm::vec2 normalizedPosition(projectedPosition.x / projectedPosition.z,
                                   projectedPosition.y / projectedPosition.z);

      if (normalizedPosition.x > xMax)
        xMax = normalizedPosition.x;
      if (normalizedPosition.x < xMin)
        xMin = normalizedPosition.x;
      if (normalizedPosition.y > yMax)
        yMax = normalizedPosition.y;
      if (normalizedPosition.y < yMin)
        yMin = normalizedPosition.y;
    }

    xMin = glm::clamp(xMin / mFrameProperties.dimensions().first, 0.0f, 1.0f);
    xMax = glm::clamp(xMax / mFrameProperties.dimensions().first, 0.0f, 1.0f);
    yMin = glm::clamp(yMin / mFrameProperties.dimensions().second, 0.0f, 1.0f);
    yMax = glm::clamp(yMax / mFrameProperties.dimensions().second, 0.0f, 1.0f);

    uint_fast16_t xMinInt = glm::floor(xMin * BASE_TEXTURE_WIDTH);
    uint_fast16_t xMaxInt = glm::ceil(xMax * BASE_TEXTURE_WIDTH);
    uint_fast16_t yMinInt = glm::floor(yMin * BASE_TEXTURE_HEIGHT);
    uint_fast16_t yMaxInt = glm::ceil(yMax * BASE_TEXTURE_HEIGHT);

    bboxVertices.push_back(std::make_tuple(xMinInt, xMaxInt, yMinInt, yMaxInt));
    size_t modelScatterPointCount = (xMaxInt - xMinInt) * (yMaxInt - yMinInt);
    fboScatterPointCount += modelScatterPointCount;
    totalScatterPointCount += modelScatterPointCount;

    if (++modelNumber % MODEL_GRID_MODEL_COUNT == 0) {
      mHistogramFboSpecs.push_back(std::make_pair(pointOffset,
                                                  fboScatterPointCount));
      pointOffset += fboScatterPointCount;
      fboScatterPointCount = 0;
    }
  }

  if (fboScatterPointCount > 0)
    mHistogramFboSpecs.push_back(std::make_pair(pointOffset,
                                                fboScatterPointCount));

  GLfloat* pointData = new GLfloat[totalScatterPointCount * 3];
  size_t pointDataOffset = 0;
  modelNumber = 0;

  for (auto& bboxVertexSet : bboxVertices) {
    auto modelGridCellNumber = (modelNumber++ % MODEL_GRID_MODEL_COUNT);
    auto modelGridLocX = modelGridCellNumber %
                         (MODELS_PER_GRID_CELL * MODEL_GRID_WIDTH);
    auto modelGridLocY = modelGridCellNumber /
                         (MODELS_PER_GRID_CELL * MODEL_GRID_WIDTH);
    uint_fast16_t xMinInt, xMaxInt, yMinInt, yMaxInt;
    std::tie(xMinInt, xMaxInt, yMinInt, yMaxInt) = bboxVertexSet;

    for (size_t i = yMinInt; i < yMaxInt; ++i) {
      for (size_t j = xMinInt; j < xMaxInt; ++j) {
        pointData[pointDataOffset++] =
            ((modelGridLocX / MODELS_PER_GRID_CELL) * BASE_TEXTURE_WIDTH + j) /
                static_cast<GLfloat>(MODEL_GRID_WIDTH * BASE_TEXTURE_WIDTH) +
            HALF_BASE_TEXEL_WIDTH;
        pointData[pointDataOffset++] =
            (modelGridLocY * BASE_TEXTURE_HEIGHT + i) /
                static_cast<GLfloat>(MODEL_GRID_HEIGHT * BASE_TEXTURE_HEIGHT) +
            HALF_BASE_TEXEL_HEIGHT;
        pointData[pointDataOffset++] = modelGridCellNumber % 2;
      }
    }
  }

  glBindBuffer(GL_ARRAY_BUFFER, mScatterPointsBuffer);
  glBufferData(GL_ARRAY_BUFFER, totalScatterPointCount * 3 * sizeof(GLfloat),
               pointData, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  assertNoGlError();

  delete[] pointData;
}


void ForegroundHistogramProcessor::setupModelGeometry(const std::vector<ModelData>& models) {
  size_t vertexCount = 0, indexCount = 0, fboIndexCount = 0;
  ptrdiff_t vertexOffset = 0;
  ptrdiff_t indexOffset = 0;
  unsigned short int modelNumber = 0;

  for (auto& model : models) {
    vertexCount += model.first.size();
    indexCount += model.second.size();
    fboIndexCount += model.second.size();

    if (++modelNumber % MODEL_GRID_MODEL_COUNT == 0) {
      mReductionFboSets.push_back(std::make_tuple(modelNumber,
                                                  indexOffset * sizeof(GLushort),
                                                  fboIndexCount));
      indexOffset += fboIndexCount;
      fboIndexCount = 0;
      modelNumber = 0;
    }
  }

  if (fboIndexCount > 0)
    mReductionFboSets.push_back(std::make_tuple(modelNumber,
                                                indexOffset * sizeof(GLushort),
                                                fboIndexCount));

  GLfloat vertexData[vertexCount * 3];
  GLushort indexData[indexCount];
  memset(vertexData, 0, sizeof(vertexData));
  indexOffset = 0;
  modelNumber = 0;

  for (auto& model : models) {
    uint_fast8_t modelColorChannel = 2 * (modelNumber % 2);
    auto modelGridCellNumber = (modelNumber % MODEL_GRID_MODEL_COUNT) /
                               MODELS_PER_GRID_CELL;
    auto modelGridLocX = modelGridCellNumber % MODEL_GRID_WIDTH;
    auto modelGridLocY = modelGridCellNumber / MODEL_GRID_WIDTH;

    for (size_t i = 0; i < model.first.size(); i += 3) {
      memcpy(vertexData + vertexOffset + i * 3, model.first.data() + i,
             3 * sizeof(GLfloat));

      vertexData[vertexOffset + i * 3 + 3 + modelColorChannel] = 1.0;
      vertexData[vertexOffset + i * 3 + 4 + modelColorChannel] = 1.0;
      vertexData[vertexOffset + i * 3 + 7] = modelGridLocX;
      vertexData[vertexOffset + i * 3 + 8] = modelGridLocY;
    }

    for (size_t i = 0; i < model.second.size(); ++i)
      indexData[indexOffset + i] = model.second[i] + vertexOffset / 9;

    modelNumber++;
    vertexOffset += model.first.size() * 3;
    indexOffset += model.second.size();
  }

  glBindBuffer(GL_ARRAY_BUFFER, mModelVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData,
               GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  assertNoGlError();

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mModelIndexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexData), indexData,
               GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  assertNoGlError();
}


const ProcessingResultSet& ForegroundHistogramProcessor::process(GLuint frameTexture) {
  auto reductionSpecIter = std::begin(mReductionFboSpecs);
  GLuint reductionGlslProgram;
  uint_fast16_t fboWidth, fboHeight;
  std::tie(reductionGlslProgram, fboWidth, fboHeight) = *reductionSpecIter;

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, frameTexture);

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);
  glBlendEquation(GL_FUNC_ADD);

  glEnableVertexAttribArray(VertexAttributeLocations::kPosition);
  glEnableVertexAttribArray(VertexAttributeLocations::kColor);
  glEnableVertexAttribArray(VertexAttributeLocations::kCellOffset);

  // Step 1: preprocessing
  glViewport(0, 0, MODEL_GRID_WIDTH * fboWidth, MODEL_GRID_HEIGHT * fboHeight);
  glUseProgram(reductionGlslProgram);
  glBindBuffer(GL_ARRAY_BUFFER, mModelVertexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mModelIndexBuffer);
  glVertexAttribPointer(VertexAttributeLocations::kPosition, 3, GL_FLOAT,
                        GL_FALSE, 9 * sizeof(GLfloat), 0);
  glVertexAttribPointer(VertexAttributeLocations::kColor, 4, GL_FLOAT,
                        GL_FALSE, 9 * sizeof(GLfloat),
                        (GLvoid*)(3 * sizeof(GLfloat)));
  glVertexAttribPointer(VertexAttributeLocations::kCellOffset, 2, GL_FLOAT,
                        GL_FALSE, 9 * sizeof(GLfloat),
                        (GLvoid*)(7 * sizeof(GLfloat)));

  auto foregroundFboIter = std::begin(mForegroundFbos);

  for (auto& reductionFboSet : mReductionFboSets) {
    glBindFramebuffer(GL_FRAMEBUFFER, *(foregroundFboIter++));
    glClear(GL_COLOR_BUFFER_BIT);

    glDrawElements(GL_TRIANGLES, std::get<2>(reductionFboSet),
                   GL_UNSIGNED_SHORT, (GLvoid*)std::get<1>(reductionFboSet));
    assertNoGlError();
  }

  // Step 2: compute histograms by scattering points
  glDisableVertexAttribArray(VertexAttributeLocations::kColor);
  glDisableVertexAttribArray(VertexAttributeLocations::kCellOffset);
  glActiveTexture(GL_TEXTURE2);
  glBindBuffer(GL_ARRAY_BUFFER, mScatterPointsBuffer);
  glVertexAttribPointer(VertexAttributeLocations::kPosition, 3, GL_FLOAT,
                        GL_FALSE, 3 * sizeof(GLfloat), 0);

  glViewport(0, 0,
             MODELS_PER_GRID_CELL * MODEL_GRID_WIDTH * HISTOGRAM_TEXTURE_WIDTH,
             MODEL_GRID_HEIGHT * HISTOGRAM_TEXTURE_HEIGHT);
  glUseProgram(mHistogramGlslProgram);

  auto foregroundTextureIter = std::begin(mForegroundTextures);
  auto histogramFboIter = std::begin(mHistogramFbos);

  for (auto& histogramFboSpec : mHistogramFboSpecs) {
    glBindTexture(GL_TEXTURE_2D, *(foregroundTextureIter++));
    glBindFramebuffer(GL_FRAMEBUFFER, *(histogramFboIter++));
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_POINTS, std::get<0>(histogramFboSpec),
                 std::get<1>(histogramFboSpec));
  }

  glDisable(GL_BLEND);
  glDisableVertexAttribArray(VertexAttributeLocations::kPosition);

  size_t modelNumber = 0;
  size_t modelCount = mModelCount;
  auto& totalPixelCounts =
      boost::get<vector<float>>(mResultSet["total_pixel_counts"]);
  auto& histogramCoverage =
      boost::get<vector<float>>(mResultSet["histogram_coverage"]);

  // Step 3: extract histograms
  for (auto histogramFbo: mHistogramFbos) {
    glBindFramebuffer(GL_FRAMEBUFFER, histogramFbo);
    GLubyte pixelData[MODELS_PER_GRID_CELL * MODEL_GRID_AREA *
                      HISTOGRAM_TEXTURE_WIDTH * HISTOGRAM_TEXTURE_HEIGHT * 4];
    glReadPixels(0, 0,
                 MODELS_PER_GRID_CELL * MODEL_GRID_WIDTH * HISTOGRAM_TEXTURE_WIDTH,
                 MODEL_GRID_HEIGHT * HISTOGRAM_TEXTURE_HEIGHT, GL_RGBA,
                 GL_UNSIGNED_BYTE, pixelData);
    uint_fast16_t offset = 0;

    for (uint_fast16_t i = 0; i < MODEL_GRID_HEIGHT; ++i) {
      vector<vector<uint_fast16_t>> histogramVectors(8);
      vector<uint_fast16_t> histogramTotals(8);

      for (uint_fast16_t j = 0; j < HISTOGRAM_TEXTURE_HEIGHT; ++j) {
        for (uint_fast16_t k = 0;
             k < MODELS_PER_GRID_CELL * MODEL_GRID_WIDTH * HISTOGRAM_TEXTURE_WIDTH;
             k += 1)
        {
          uint_fast8_t histogramIndex = k / HISTOGRAM_TEXTURE_WIDTH;
          uint_fast16_t bucketValue = pixelData[offset] +
                                      pixelData[offset + 1] +
                                      pixelData[offset + 2] +
                                      pixelData[offset + 3];

          histogramVectors[histogramIndex].push_back(bucketValue);
          histogramTotals[histogramIndex] += bucketValue;
          offset += 4;
        }
      }

      for (uint_fast8_t j = 0; j < std::min(modelCount, 8u); ++j) {
        auto& resultHistogram = histogramVectors[j];
        auto& normalizedHistogram =
            boost::get<vector<float>>(mResultSet[std::to_string(modelNumber)]);
        float histogramTotal = histogramTotals[j];

        histogramCoverage[modelNumber] =
            histogramTotal / mModelAreas[modelNumber];
        totalPixelCounts[modelNumber++] = histogramTotal;

        if (histogramTotal == 0) {
          for (uint_fast16_t k = 0; k < HISTOGRAM_TEXTURE_AREA; ++k)
            normalizedHistogram[k] = 0.0f;
        } else {
          for (uint_fast16_t k = 0; k < HISTOGRAM_TEXTURE_AREA; ++k)
            normalizedHistogram[k] = resultHistogram[k] / histogramTotal;
        }
      }

      if (modelCount < 8)
        return mResultSet;

      modelCount -= 8;
    }
  }

  return mResultSet;
}


} // end namespace processors
} // end namespace glipf
