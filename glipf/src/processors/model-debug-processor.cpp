#include <glipf/processors/model-debug-processor.h>

#include <glipf/gles-utils/shader-builder.h>
#include <glipf/gles-utils/glsl-program-builder.h>

#include <boost/variant/get.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstring>


using std::vector;


namespace glipf {
namespace processors {


enum VertexAttributeLocations : GLuint {
  kPosition = 0,
  kColor = 1
};


ModelDebugProcessor::ModelDebugProcessor(const sources::FrameProperties& frameProperties,
                                         const glm::mat4& mvpMatrix)
  : GlesProcessor(frameProperties)
  , mPassthroughGlslProgram(0)
  , mMainGlslProgram(0)
  , mModelIndexCount(0)
  , mModelVertexBuffer(0)
  , mModelIndexBuffer(0)
{
  mPassthroughGlslProgram = gles_utils::GlslProgramBuilder()
    .attachShader(gles_utils::ShaderBuilder(GL_VERTEX_SHADER)
                    .appendSourceFile("glsl/standard.vert")
                    .compile())
    .attachShader(gles_utils::ShaderBuilder(GL_FRAGMENT_SHADER)
                    .appendSourceFile("glsl/noop.frag")
                    .compile())
    .bindAttribLocation(VertexAttributeLocations::kPosition, "vertex")
    .link();

  glUseProgram(mPassthroughGlslProgram);
  glUniform1i(glGetUniformLocation(mPassthroughGlslProgram, "tex"), 0);

  mMainGlslProgram = gles_utils::GlslProgramBuilder()
    .attachShader(gles_utils::ShaderBuilder(GL_VERTEX_SHADER)
                    .appendSourceFile("glsl/model-debug/transformation.vert")
                    .compile())
    .attachShader(gles_utils::ShaderBuilder(GL_FRAGMENT_SHADER)
                    .appendSourceFile("glsl/model-debug/model-color.frag")
                    .compile())
    .bindAttribLocation(VertexAttributeLocations::kPosition, "vertex")
    .bindAttribLocation(VertexAttributeLocations::kColor, "vertexColor")
    .link();

  glUseProgram(mMainGlslProgram);
  glUniform2f(glGetUniformLocation(mMainGlslProgram, "viewportDimensions"),
              frameProperties.dimensions().first,
              frameProperties.dimensions().second);
  glUniformMatrix4fv(glGetUniformLocation(mMainGlslProgram, "projectionMatrix"),
                     1, GL_FALSE, glm::value_ptr(mvpMatrix));
  assertNoGlError();

  // Prepare a texture to store the model debug image
  GLuint modelDebugTexture;
  glActiveTexture(GL_TEXTURE3);
  glGenTextures(1, &modelDebugTexture);
  glBindTexture(GL_TEXTURE_2D, modelDebugTexture);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frameProperties.dimensions().first,
               frameProperties.dimensions().second, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, 0);
  assertNoGlError();

  // Prepare an FBO to store the model debug image
  GLuint modelDebugFbo;
  glGenFramebuffers(1, &modelDebugFbo);
  glBindFramebuffer(GL_FRAMEBUFFER, modelDebugFbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_2D, modelDebugTexture, 0);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  assertNoGlError();

  mTextureFboPair = std::make_pair(modelDebugTexture, modelDebugFbo);

  glGenBuffers(1, &mModelVertexBuffer);
  glGenBuffers(1, &mModelIndexBuffer);
  assertNoGlError();

  mResultSet["model_debug"] = modelDebugTexture;
}


ModelDebugProcessor::~ModelDebugProcessor() {
  glDeleteProgram(mPassthroughGlslProgram);
  glDeleteProgram(mMainGlslProgram);
  glDeleteFramebuffers(1, &std::get<1>(mTextureFboPair));
  glDeleteTextures(1, &std::get<0>(mTextureFboPair));
  glDeleteBuffers(1, &mModelVertexBuffer);
  glDeleteBuffers(1, &mModelIndexBuffer);
}


glm::vec4 ModelDebugProcessor::getGroupColor(uint_fast8_t groupId) {
  switch (groupId) {
    case 0:
      return glm::vec4(220.0 / 255.0, 50.0 / 255.0, 47.0 / 255.0, 0.75);
    case 1:
      return glm::vec4(38.0 / 255.0, 139.0 / 255.0, 210.0 / 255.0, 0.25);
    case 2:
      return glm::vec4(181.0 / 255.0, 137.0 / 255.0, 0.0, 0.25);
    case 3:
      return glm::vec4(133.0 / 255.0, 153.0 / 255.0, 0.0 / 255.0, 0.25);
    case 4:
      return glm::vec4(108.0 / 255.0, 113.0 / 255.0, 196.0 / 255.0, 0.25);
    case 5:
      return glm::vec4(7.0 / 255.0, 54.0 / 255.0, 66.0 / 255.0, 0.25);
    case 6:
      return glm::vec4(42.0 / 255.0, 161.0 / 255.0, 152.0 / 255.0, 0.25);
    default:
      return glm::vec4(0.0, 0.0, 0.0, 0.25);
  };
}


void ModelDebugProcessor::setModels(const std::vector<ModelData>& models,
                                    const std::vector<uint_fast8_t>& modelGroups)
{
  size_t vertexCount = 0, indexCount = 0;
  ptrdiff_t vertexOffset = 0;
  ptrdiff_t indexOffset = 0;
  unsigned short int modelNumber = 0;

  for (auto& model : models) {
    vertexCount += model.first.size();
    indexCount += model.second.size();
  }

  GLfloat vertexData[(vertexCount / 3) * 7];
  GLushort indexData[indexCount];

  for (auto& model : models) {
    for (size_t i = 0; i < model.second.size(); ++i)
      indexData[indexOffset + i] = model.second[i] + vertexOffset / 7;

    for (size_t i = 0; i < model.first.size(); i += 3) {
      memcpy(vertexData + vertexOffset, model.first.data() + i,
             3 * sizeof(GLfloat));
      uint_fast8_t groupId = 0;

      if (modelGroups.size() > modelNumber)
        groupId = modelGroups[modelNumber];

      glm::vec4 groupColor = getGroupColor(groupId);

      vertexData[vertexOffset + 3] = groupColor[0];
      vertexData[vertexOffset + 4] = groupColor[1];
      vertexData[vertexOffset + 5] = groupColor[2];
      vertexData[vertexOffset + 6] = groupColor[3];

      vertexOffset += 7;
    }

    modelNumber++;
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


const ProcessingResultSet& ModelDebugProcessor::process(GLuint frameTexture) {
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, frameTexture);

  glEnableVertexAttribArray(VertexAttributeLocations::kPosition);

  glViewport(0, 0, mFrameProperties.dimensions().first,
             mFrameProperties.dimensions().second);
  glUseProgram(mPassthroughGlslProgram);
  glBindFramebuffer(GL_FRAMEBUFFER, std::get<1>(mTextureFboPair));
  glClear(GL_COLOR_BUFFER_BIT);
  drawFullscreenQuad(VertexAttributeLocations::kPosition);

  glEnableVertexAttribArray(VertexAttributeLocations::kColor);
  glBindBuffer(GL_ARRAY_BUFFER, mModelVertexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mModelIndexBuffer);
  glVertexAttribPointer(VertexAttributeLocations::kPosition, 3, GL_FLOAT,
                        GL_FALSE, 7 * sizeof(GLfloat), 0);
  glVertexAttribPointer(VertexAttributeLocations::kColor, 4, GL_FLOAT,
                        GL_FALSE, 7 * sizeof(GLfloat),
                        (GLvoid*)(3 * sizeof(GLfloat)));

  glEnable(GL_BLEND);
  glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);

  glUseProgram(mMainGlslProgram);
  glDrawElements(GL_LINES, mModelIndexCount, GL_UNSIGNED_SHORT, 0);
  assertNoGlError();

  glDisable(GL_BLEND);
  glDisableVertexAttribArray(VertexAttributeLocations::kPosition);
  glDisableVertexAttribArray(VertexAttributeLocations::kColor);

  return mResultSet;
}


} // end namespace processors
} // end namespace glipf
