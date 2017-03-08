#include <glipf/processors/foreground-coverage-processor.h>

#include <glipf/gles-utils/shader-builder.h>
#include <glipf/gles-utils/glsl-program-builder.h>

#include <boost/variant/get.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstring>


#define BASE_TEXTURE_WIDTH 320
#define BASE_TEXTURE_HEIGHT 240


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


ForegroundCoverageProcessor::ForegroundCoverageProcessor(const sources::FrameProperties& frameProperties,
                                                         const vector<ModelData>& models,
                                                         const glm::mat4& mvpMatrix)
  : GlesProcessor(frameProperties)
  , mPixelCountingGlslProgram(0)
  , mModelVertexBuffer(0)
  , mModelIndexBuffer(0)
{
  setupReductionGlslPrograms(mvpMatrix);
  setupModelGeometry(models);

  mResultSet["model_coverage"] = vector<float>();
}


ForegroundCoverageProcessor::~ForegroundCoverageProcessor() {
  glDeleteProgram(mPixelCountingGlslProgram);
  glDeleteBuffers(1, &mModelVertexBuffer);
  glDeleteBuffers(1, &mModelIndexBuffer);

  for (auto reductionFboSpec : mReductionFboSpecs)
    glDeleteProgram(std::get<0>(reductionFboSpec));

  for (const auto& reductionFboSet : mReductionFboSets) {
    for (auto modelTextureFboPair : std::get<3>(reductionFboSet)) {
      glDeleteFramebuffers(1, &std::get<1>(modelTextureFboPair));
      glDeleteTextures(1, &std::get<0>(modelTextureFboPair));
    }
  }
}


void ForegroundCoverageProcessor::setupReductionGlslPrograms(const glm::mat4& mvpMatrix)
{
  GLuint mainGlslProgram = gles_utils::GlslProgramBuilder()
    .attachShader(gles_utils::ShaderBuilder(GL_VERTEX_SHADER)
                    .appendSourceFile("glsl/transformation.vert")
                    .compile())
    .attachShader(gles_utils::ShaderBuilder(GL_FRAGMENT_SHADER)
                    .appendSourceFile("glsl/foreground-coverage.frag")
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

  vector<tuple<uint16_t, uint16_t, uint16_t, uint16_t>> reductionFboSpecs = {
    std::make_tuple(BASE_TEXTURE_WIDTH / 4, BASE_TEXTURE_HEIGHT / 4, 4, 4),
    std::make_tuple(BASE_TEXTURE_WIDTH / 16, BASE_TEXTURE_HEIGHT / 16, 4, 4),
    std::make_tuple(BASE_TEXTURE_WIDTH / 80, BASE_TEXTURE_HEIGHT / 80, 5, 5)
  };

  for (auto& reductionFboSpec : reductionFboSpecs) {
    uint16_t fboWidth, fboHeight, texelWidth, texelHeight;
    std::tie(fboWidth, fboHeight, texelWidth, texelHeight) = reductionFboSpec;

    GLuint reductionGlslProgram = gles_utils::GlslProgramBuilder()
      .attachShader(gles_utils::ShaderBuilder(GL_VERTEX_SHADER)
                      .appendSourceFile("glsl/active-pixel-count.vert")
                      .compile())
      .attachShader(gles_utils::ShaderBuilder(GL_FRAGMENT_SHADER)
                      .appendSourceString("#define TEXEL_WIDTH " +
                                          std::to_string(texelWidth) + ".0\n")
                      .appendSourceString("#define TEXEL_HEIGHT " +
                                          std::to_string(texelHeight) + ".0\n")
                      .appendSourceFile("glsl/active-pixel-count.frag")
                      .compile())
      .bindAttribLocation(VertexAttributeLocations::kPosition, "vertex")
      .link();

    glUseProgram(reductionGlslProgram);
    glUniform1i(glGetUniformLocation(reductionGlslProgram, "tex"), 2);
    glUniform2f(glGetUniformLocation(reductionGlslProgram, "stepSize"),
                0.5f / (4 * texelWidth * fboWidth),
                0.5f / (4 * texelHeight * fboHeight));
    assertNoGlError();

    mReductionFboSpecs.push_back(std::make_tuple(reductionGlslProgram,
                                                 fboWidth,
                                                 fboHeight));
  }
}


void ForegroundCoverageProcessor::addReductionFboSet(size_t modelCount,
                                                     GLuint indexOffset,
                                                     GLuint indexCount)
{
  vector<TextureFboPair> reductionObjects;

  // Prepare a texture to store the average foreground coverage of the
  // model
  for (auto& spec : mReductionFboSpecs) {
    GLuint averageTexture;
    glActiveTexture(GL_TEXTURE3);
    glGenTextures(1, &averageTexture);
    glBindTexture(GL_TEXTURE_2D, averageTexture);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4 * std::get<1>(spec),
                 4 * std::get<2>(spec), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    assertNoGlError();

    // Prepare an FBO to store the average foreground coverage of the
    // model
    GLuint averageFbo;
    glGenFramebuffers(1, &averageFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, averageFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, averageTexture, 0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    assertNoGlError();

    reductionObjects.push_back(std::make_pair(averageTexture, averageFbo));
  }

  mReductionFboSets.push_back(std::make_tuple(modelCount,
                                              indexOffset * sizeof(GLushort),
                                              indexCount, reductionObjects));
}


void ForegroundCoverageProcessor::setupModelGeometry(const std::vector<ModelData>& models) {
  size_t vertexCount = 0, indexCount = 0, fboIndexCount = 0;
  ptrdiff_t vertexOffset = 0;
  ptrdiff_t indexOffset = 0;
  unsigned short int modelNumber = 0;

  for (auto& model : models) {
    vertexCount += model.first.size();
    indexCount += model.second.size();
    fboIndexCount += model.second.size();

    if (++modelNumber % 32 == 0) {
      addReductionFboSet(modelNumber, indexOffset, fboIndexCount);
      indexOffset += fboIndexCount;
      fboIndexCount = 0;
      modelNumber = 0;
    }
  }

  if (fboIndexCount > 0)
    addReductionFboSet(modelNumber, indexOffset, fboIndexCount);

  GLfloat vertexData[vertexCount * 3];
  GLushort indexData[indexCount];
  memset(vertexData, 0, sizeof(vertexData));
  indexOffset = 0;
  modelNumber = 0;

  for (auto& model : models) {
    uint_fast8_t modelColorChannel = 2 * (modelNumber % 2);

    for (size_t i = 0; i < model.first.size(); i += 3) {
      memcpy(vertexData + vertexOffset + i * 3, model.first.data() + i,
             3 * sizeof(GLfloat));

      vertexData[vertexOffset + i * 3 + 3 + modelColorChannel] = 1.0;
      vertexData[vertexOffset + i * 3 + 4 + modelColorChannel] = 1.0;
      vertexData[vertexOffset + i * 3 + 7] = ((modelNumber / 2) % 16) % 4;
      vertexData[vertexOffset + i * 3 + 8] = ((modelNumber / 2) % 16) / 4;
    }

    for (size_t i = 0; i < model.second.size(); ++i)
      indexData[indexOffset + i] = model.second[i] + vertexOffset / 9;

    modelNumber++;
    vertexOffset += model.first.size() * 3;
    indexOffset += model.second.size();
  }

  glGenBuffers(1, &mModelVertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, mModelVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData,
               GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  assertNoGlError();

  glGenBuffers(1, &mModelIndexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mModelIndexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexData), indexData,
               GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  assertNoGlError();
}


const ProcessingResultSet& ForegroundCoverageProcessor::process(GLuint frameTexture) {
  vector<float>& modelCoverageSet =
      boost::get<vector<float>>(mResultSet["model_coverage"]);
  modelCoverageSet.clear();

  auto reductionSpecIter = std::begin(mReductionFboSpecs);
  GLuint reductionGlslProgram;
  uint_fast16_t fboWidth, fboHeight;
  std::tie(reductionGlslProgram, fboWidth, fboHeight) = *reductionSpecIter;

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);
  glBlendEquation(GL_FUNC_ADD);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, frameTexture);

  glEnableVertexAttribArray(VertexAttributeLocations::kPosition);
  glEnableVertexAttribArray(VertexAttributeLocations::kColor);
  glEnableVertexAttribArray(VertexAttributeLocations::kCellOffset);

  // Step 1: preprocessing
  glViewport(0, 0, 4 * fboWidth, 4 * fboHeight);
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

  for (auto& reductionFboSet : mReductionFboSets) {
    const auto& modelTextureFboPair = std::get<3>(reductionFboSet).front();

    glBindFramebuffer(GL_FRAMEBUFFER, std::get<1>(modelTextureFboPair));
    glClear(GL_COLOR_BUFFER_BIT);

    glDrawElements(GL_TRIANGLES, std::get<2>(reductionFboSet),
                   GL_UNSIGNED_SHORT, (GLvoid*)std::get<1>(reductionFboSet));
    assertNoGlError();
  }

  glDisable(GL_BLEND);
  glDisableVertexAttribArray(VertexAttributeLocations::kColor);
  glDisableVertexAttribArray(VertexAttributeLocations::kCellOffset);

  // Step 2: average coverage by mipmapping
  glActiveTexture(GL_TEXTURE2);
  size_t reductionFboIndex = 1;

  while (++reductionSpecIter != std::end(mReductionFboSpecs)) {
    std::tie(reductionGlslProgram, fboWidth, fboHeight) = *reductionSpecIter;
    glViewport(0, 0, 4 * fboWidth, 4 * fboHeight);
    glUseProgram(reductionGlslProgram);

    for (auto& reductionFboSet : mReductionFboSets) {
      const auto& textureFboPairList = std::get<3>(reductionFboSet);

      glBindTexture(GL_TEXTURE_2D,
                    std::get<0>(textureFboPairList[reductionFboIndex - 1]));
      glBindFramebuffer(GL_FRAMEBUFFER,
                        std::get<1>(textureFboPairList[reductionFboIndex]));
      glClear(GL_COLOR_BUFFER_BIT);
      drawFullscreenQuad(VertexAttributeLocations::kPosition);
    }

    reductionFboIndex++;
  }

  glDisableVertexAttribArray(VertexAttributeLocations::kPosition);

  // Step 3: extract coverage
  for (auto& reductionFboSet : mReductionFboSets) {
    glBindFramebuffer(GL_FRAMEBUFFER,
                      std::get<1>(std::get<3>(reductionFboSet).back()));
    GLubyte pixelData[fboWidth * fboHeight * 64];
    glReadPixels(0, 0, 4 * fboWidth, 4 * fboHeight, GL_RGBA, GL_UNSIGNED_BYTE,
                 pixelData);
    uint_fast16_t offset = 0;
    size_t modelCount = std::get<0>(reductionFboSet);

    for (uint_fast16_t i = 0; i < 4; ++i) {
      uint_fast32_t modelCoverage[] = {0, 0, 0, 0, 0, 0, 0, 0};
      uint_fast32_t foregroundCoverage[] = {0, 0, 0, 0, 0, 0, 0, 0};

      for (uint_fast16_t j = 0; j < fboHeight; ++j) {
        for (uint_fast16_t k = 0; k < 4 * fboWidth; ++k) {
          uint_fast8_t modelIndex = (k / fboWidth) * 2;
          modelCoverage[modelIndex] += pixelData[offset++];
          foregroundCoverage[modelIndex] += pixelData[offset++];
          modelCoverage[modelIndex + 1] += pixelData[offset++];
          foregroundCoverage[modelIndex + 1] += pixelData[offset++];
        }
      }

      for (uint_fast16_t j = 0; j < std::min(modelCount, 8u); ++j)
        modelCoverageSet.push_back(foregroundCoverage[j] / (float)modelCoverage[j]);

      if (modelCount < 8)
        return mResultSet;

      modelCount -= 8;
    }
  }

  return mResultSet;
}


} // end namespace processors
} // end namespace glipf
