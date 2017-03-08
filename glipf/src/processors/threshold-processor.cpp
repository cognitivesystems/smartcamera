#include <glipf/processors/threshold-processor.h>

#include <glm/gtc/type_ptr.hpp>

#include <glipf/gles-utils/shader-builder.h>
#include <glipf/gles-utils/glsl-program-builder.h>


namespace glipf {
namespace processors {


enum VertexAttributeLocations : GLuint {
  kPosition = 0
};


ThresholdProcessor::ThresholdProcessor(const sources::FrameProperties& frameProperties,
                                       glm::vec3 lowerHsvThreshold,
                                       glm::vec3 upperHsvThreshold)
  : GlesProcessor(frameProperties)
  , mGlslProgram(0)
{
  mGlslProgram = gles_utils::GlslProgramBuilder()
    .attachShader(gles_utils::ShaderBuilder(GL_VERTEX_SHADER)
                    .appendSourceFile("glsl/standard.vert")
                    .compile())
    .attachShader(gles_utils::ShaderBuilder(GL_FRAGMENT_SHADER)
                    .appendSourceFile("glsl/include/color-space.frag")
                    .appendSourceFile("glsl/threshold.frag")
                    .compile())
    .bindAttribLocation(VertexAttributeLocations::kPosition, "vertex")
    .link();

  glUseProgram(mGlslProgram);
  glUniform1i(glGetUniformLocation(mGlslProgram, "tex"), 0);
  glUniform3fv(glGetUniformLocation(mGlslProgram, "lowerHsvThreshold"),
               1, glm::value_ptr(lowerHsvThreshold));
  glUniform3fv(glGetUniformLocation(mGlslProgram, "upperHsvThreshold"),
               1, glm::value_ptr(upperHsvThreshold));
  assertNoGlError();

  std::tie(mResultTexture, mResultFbo) =
      generateTextureBackedFbo(frameProperties.dimensions());
  mResultSet["thresholded_texture"] = mResultTexture;
}


ThresholdProcessor::~ThresholdProcessor() {
  glDeleteProgram(mGlslProgram);
  glDeleteFramebuffers(1, &mResultFbo);
  glDeleteTextures(1, &mResultTexture);
}


const ProcessingResultSet& ThresholdProcessor::process(GLuint frameTexture) {
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, frameTexture);

  glEnableVertexAttribArray(VertexAttributeLocations::kPosition);

  glBindFramebuffer(GL_FRAMEBUFFER, mResultFbo);
  glViewport(0, 0, mFrameProperties.dimensions().first,
             mFrameProperties.dimensions().second);
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(mGlslProgram);
  drawFullscreenQuad(VertexAttributeLocations::kPosition);

  glDisableVertexAttribArray(VertexAttributeLocations::kPosition);

  return mResultSet;
}


} // end namespace processors
} // end namespace glipf
