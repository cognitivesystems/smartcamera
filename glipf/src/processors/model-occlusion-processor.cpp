#include <glipf/processors/model-occlusion-processor.h>

#include <glipf/gles-utils/shader-builder.h>
#include <glipf/gles-utils/glsl-program-builder.h>

#include <boost/variant/get.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstring>


#define BASE_TEXTURE_WIDTH 320
#define BASE_TEXTURE_HEIGHT 240

constexpr GLfloat kColorUnitValue = 1.0 / 256.0;


using std::vector;


namespace glipf {
namespace processors {


enum VertexAttributeLocations : GLuint {
  kPosition = 0,
  kColor = 1
};


ModelOcclusionProcessor::ModelOcclusionProcessor(const sources::FrameProperties& frameProperties,
                                                 const glm::mat4& mvpMatrix)
  : GlesProcessor(frameProperties)
  , mModelCount(0)
  , mMainGlslProgram(0)
  , mModelIndexCount(0)
  , mModelVertexBuffer(0)
  , mModelIndexBuffer(0)
{
  mMainGlslProgram = gles_utils::GlslProgramBuilder()
    .attachShader(gles_utils::ShaderBuilder(GL_VERTEX_SHADER)
                    .appendSourceFile("glsl/model-occlusion/transformation.vert")
                    .compile())
    .attachShader(gles_utils::ShaderBuilder(GL_FRAGMENT_SHADER)
                    .appendSourceFile("glsl/model-occlusion/model-color.frag")
                    .compile())
    .bindAttribLocation(VertexAttributeLocations::kPosition, "vertex")
    .bindAttribLocation(VertexAttributeLocations::kColor, "modelColor")
    .link();

  glUseProgram(mMainGlslProgram);
  glUniform2f(glGetUniformLocation(mMainGlslProgram, "viewportDimensions"),
              frameProperties.dimensions().first,
              frameProperties.dimensions().second);
  glUniformMatrix4fv(glGetUniformLocation(mMainGlslProgram, "projectionMatrix"),
                     1, GL_FALSE, glm::value_ptr(mvpMatrix));
  assertNoGlError();

  // Prepare a texture to store the model occlusion image
  glActiveTexture(GL_TEXTURE3);
  glGenTextures(1, &mTexture);
  glBindTexture(GL_TEXTURE_2D, mTexture);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, BASE_TEXTURE_WIDTH,
               BASE_TEXTURE_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
  assertNoGlError();

  // Prepare a renderbuffer for depth testing
  glGenRenderbuffers(1, &mRenderBuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, mRenderBuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                        BASE_TEXTURE_WIDTH, BASE_TEXTURE_HEIGHT);

  // Prepare an FBO to store the model occlusion image
  glGenFramebuffers(1, &mFrameBuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_2D, mTexture, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, mRenderBuffer);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  assertNoGlError();

  glGenBuffers(1, &mModelVertexBuffer);
  glGenBuffers(1, &mModelIndexBuffer);
  assertNoGlError();

  mResultSet["model_occlusion_texture"] = mTexture;
  mResultSet["model_occlusion"] = vector<float>();
}


ModelOcclusionProcessor::~ModelOcclusionProcessor() {
  glDeleteProgram(mMainGlslProgram);
  glDeleteTextures(1, &mTexture);
  glDeleteRenderbuffers(1, &mRenderBuffer);
  glDeleteFramebuffers(1, &mFrameBuffer);
  glDeleteBuffers(1, &mModelVertexBuffer);
  glDeleteBuffers(1, &mModelIndexBuffer);
}


void ModelOcclusionProcessor::setModels(const vector<ModelData>& models,
                                        const glm::mat4& mvpMatrix)
{
  mModelCount = models.size();
  mModelAreas = computeModelAreas(models, mvpMatrix, BASE_TEXTURE_WIDTH,
                                  BASE_TEXTURE_HEIGHT);
  setupModelGeometry(models);

  vector<float>& occlusionValues =
      boost::get<vector<float>>(mResultSet["model_occlusion"]);
  occlusionValues.resize(mModelCount);
}


void ModelOcclusionProcessor::setupModelGeometry(const std::vector<ModelData>& models) {
  size_t vertexCount = 0, indexCount = 0;
  ptrdiff_t vertexOffset = 0;
  ptrdiff_t indexOffset = 0;
  unsigned short int modelNumber = 0;

  for (auto& model : models) {
    vertexCount += model.first.size();
    indexCount += model.second.size();
  }

  GLfloat vertexData[vertexCount * 2];
  GLushort indexData[indexCount];

  for (auto& model : models) {
    GLfloat modelColor = (modelNumber + 1) * kColorUnitValue;

    for (size_t i = 0; i < model.first.size(); i += 3) {
      memcpy(vertexData + vertexOffset + i * 2, model.first.data() + i,
             3 * sizeof(GLfloat));

      vertexData[vertexOffset + i * 2 + 3] = modelColor;
      vertexData[vertexOffset + i * 2 + 4] = modelColor;
      vertexData[vertexOffset + i * 2 + 5] = modelColor;
    }

    for (size_t i = 0; i < model.second.size(); ++i)
      indexData[indexOffset + i] = model.second[i] + vertexOffset / 6;

    modelNumber++;
    vertexOffset += model.first.size() * 2;
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

  mModelIndexCount = sizeof(indexData) / sizeof(GLushort);
}


const ProcessingResultSet& ModelOcclusionProcessor::process(GLuint /*frameTexture*/) {
  glEnableVertexAttribArray(VertexAttributeLocations::kPosition);
  glEnableVertexAttribArray(VertexAttributeLocations::kColor);

  glViewport(0, 0, BASE_TEXTURE_WIDTH, BASE_TEXTURE_HEIGHT);
  glUseProgram(mMainGlslProgram);
  glBindBuffer(GL_ARRAY_BUFFER, mModelVertexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mModelIndexBuffer);
  glVertexAttribPointer(VertexAttributeLocations::kPosition, 3, GL_FLOAT,
                        GL_FALSE, 6 * sizeof(GLfloat), 0);
  glVertexAttribPointer(VertexAttributeLocations::kColor, 3, GL_FLOAT,
                        GL_FALSE, 6 * sizeof(GLfloat),
                        (GLvoid*)(3 * sizeof(GLfloat)));

  glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  glDrawElements(GL_TRIANGLES, mModelIndexCount, GL_UNSIGNED_SHORT, 0);
  assertNoGlError();
  glDisable(GL_DEPTH_TEST);
  glDisableVertexAttribArray(VertexAttributeLocations::kPosition);
  glDisableVertexAttribArray(VertexAttributeLocations::kColor);

  GLubyte pixelData[BASE_TEXTURE_WIDTH * BASE_TEXTURE_HEIGHT * 4];
  glReadPixels(0, 0, BASE_TEXTURE_WIDTH, BASE_TEXTURE_HEIGHT, GL_RGBA,
               GL_UNSIGNED_BYTE, pixelData);

  vector<uint_fast16_t> modelPixelCounts(mModelCount);

  for (size_t i = 0; i < sizeof(pixelData); i += 4) {
    if (pixelData[i] > 0)
      modelPixelCounts[pixelData[i] - 1]++;
  }

  vector<float>& occlusionValues =
      boost::get<vector<float>>(mResultSet["model_occlusion"]);

  for (size_t i = 0; i < mModelCount; ++i)
    occlusionValues[i] = modelPixelCounts[i] / mModelAreas[i];

  return mResultSet;
}


} // end namespace processors
} // end namespace glipf
