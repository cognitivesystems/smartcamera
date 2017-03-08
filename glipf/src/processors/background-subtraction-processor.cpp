#include <glipf/processors/background-subtraction-processor.h>

#include <glipf/gles-utils/shader-builder.h>
#include <glipf/gles-utils/glsl-program-builder.h>


namespace glipf {
namespace processors {


enum VertexAttributeLocations : GLuint {
  kPosition = 0
};


BackgroundSubtractionProcessor::BackgroundSubtractionProcessor(const sources::FrameProperties& frameProperties,
                                                               const void* referenceFrameData)
  : GlesProcessor(frameProperties)
  , mGlslProgram(0)
  , mReferenceFrameTexture(0)
  , mResultTexture(0)
  , mResultFbo(0)
{
  // Prepare a reference frame texture image
  glActiveTexture(GL_TEXTURE1);
  glGenTextures(1, &mReferenceFrameTexture);
  glBindTexture(GL_TEXTURE_2D, mReferenceFrameTexture);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frameProperties.dimensions().first,
               frameProperties.dimensions().second, 0, GL_RGB,
               GL_UNSIGNED_BYTE, referenceFrameData);
  assertNoGlError();

  mGlslProgram = gles_utils::GlslProgramBuilder()
    .attachShader(gles_utils::ShaderBuilder(GL_VERTEX_SHADER)
                    .appendSourceFile("glsl/standard.vert")
                    .compile())
    .attachShader(gles_utils::ShaderBuilder(GL_FRAGMENT_SHADER)
                    .appendSourceFile("glsl/include/color-space.frag")
                    .appendSourceFile("glsl/include/background-foreground.frag")
                    .appendSourceFile("glsl/background-subtraction.frag")
                    .compile())
    .bindAttribLocation(VertexAttributeLocations::kPosition, "vertex")
    .link();

  glUseProgram(mGlslProgram);
  glUniform1i(glGetUniformLocation(mGlslProgram, "tex"), 0);
  glUniform1i(glGetUniformLocation(mGlslProgram, "referenceFrameTexture"), 1);

  setupResultFbo();
}


BackgroundSubtractionProcessor::~BackgroundSubtractionProcessor() {
  glDeleteProgram(mGlslProgram);
  glDeleteFramebuffers(1, &mResultFbo);
  glDeleteTextures(1, &mResultTexture);
  glDeleteTextures(1, &mReferenceFrameTexture);
}


void BackgroundSubtractionProcessor::setupResultFbo()
{
  // Prepare a texture to store the background-subtracted image
  glActiveTexture(GL_TEXTURE3);
  glGenTextures(1, &mResultTexture);
  glBindTexture(GL_TEXTURE_2D, mResultTexture);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mFrameProperties.dimensions().first,
               mFrameProperties.dimensions().second, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, 0);
  assertNoGlError();

  // Prepare an FBO to store the background-subtracted image image
  glGenFramebuffers(1, &mResultFbo);
  glBindFramebuffer(GL_FRAMEBUFFER, mResultFbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         mResultTexture, 0);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  assertNoGlError();

  mResultSet["foreground_texture"] = mResultTexture;
}


const ProcessingResultSet& BackgroundSubtractionProcessor::process(GLuint frameTexture) {
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
