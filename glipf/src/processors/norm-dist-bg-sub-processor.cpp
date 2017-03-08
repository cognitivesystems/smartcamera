#include <glipf/processors/norm-dist-bg-sub-processor.h>

#include <glipf/gles-utils/shader-builder.h>
#include <glipf/gles-utils/glsl-program-builder.h>

#include <glm/gtx/color_space_YCoCg.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstring>


using std::vector;


namespace glipf {
namespace processors {


enum VertexAttributeLocations : GLuint {
  kPosition = 0
};


NormDistBgSubProcessor::NormDistBgSubProcessor(const sources::FrameProperties& frameProperties)
  : GlesProcessor(frameProperties)
  , mGlslProgram(0)
  , mReferenceFrameTexture(0)
  , mMeanTexture(0)
  , mStdDevTexture(0)
  , mResultTexture(0)
  , mResultFbo(0)
{
  mGlslProgram = gles_utils::GlslProgramBuilder()
    .attachShader(gles_utils::ShaderBuilder(GL_VERTEX_SHADER)
                    .appendSourceFile("glsl/standard.vert")
                    .compile())
    .attachShader(gles_utils::ShaderBuilder(GL_FRAGMENT_SHADER)
                    .appendSourceFile("glsl/include/color-space.frag")
                    .appendSourceFile("glsl/norm-dist-bg-sub/subtraction.frag")
                    .compile())
    .bindAttribLocation(VertexAttributeLocations::kPosition, "position")
    .link();

  glUseProgram(mGlslProgram);
  glUniform1i(glGetUniformLocation(mGlslProgram, "frameTexture"), 0);
  glUniform1i(glGetUniformLocation(mGlslProgram, "meanTexture"), 1);
  glUniform1i(glGetUniformLocation(mGlslProgram, "stdDevTexture"), 2);

  setupResultFbo();
}


NormDistBgSubProcessor::~NormDistBgSubProcessor() {
  glDeleteProgram(mGlslProgram);
  glDeleteFramebuffers(1, &mResultFbo);
  glDeleteTextures(1, &mResultTexture);
  glDeleteTextures(1, &mReferenceFrameTexture);
  glDeleteTextures(1, &mMeanTexture);
  glDeleteTextures(1, &mStdDevTexture);

  for (auto sampleData : mBackgroundSamples)
    delete[] sampleData;
}


void NormDistBgSubProcessor::setupResultFbo()
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
}


void NormDistBgSubProcessor::addBackgroundSample(const void* frameData) {
  size_t dataLen = mFrameProperties.dimensions().first *
      mFrameProperties.dimensions().second * 3;
  uint8_t* sampleData = new uint8_t[dataLen];
  memcpy(sampleData, frameData, dataLen);

  mBackgroundSamples.push_back(sampleData);
}


void NormDistBgSubProcessor::setupBackgroundModel() {
  size_t pixelCount = mFrameProperties.dimensions().first *
      mFrameProperties.dimensions().second;
  uint8_t meanTextureData[pixelCount * 3];
  uint8_t stdDevTextureData[pixelCount * 3];
  ptrdiff_t textureDataOffset = 0;
  float sampleCount = mBackgroundSamples.size();

  for (size_t i = 0; i < pixelCount * 3; i += 3) {
    vector<glm::vec3> samples;
    glm::vec3 yCoCgPixelColorSum(0, 0, 0);

    for (auto sampleData : mBackgroundSamples) {
      glm::vec3 rgbPixelColor(*(sampleData + i + 2) / 255.0f,
                              *(sampleData + i + 1) / 255.0f,
                              *(sampleData + i) / 255.0f);
      samples.push_back(glm::rgb2YCoCg(rgbPixelColor));
      yCoCgPixelColorSum += samples.back();
    }

    glm::vec3 yCoCgPixelColorMean = yCoCgPixelColorSum / sampleCount;
    glm::mat3 covarianceMatrix(0);

    for (const auto& sample : samples) {
      glm::vec3 colorDelta = sample - yCoCgPixelColorMean;
      covarianceMatrix += glm::outerProduct(colorDelta, colorDelta);
    }

    covarianceMatrix /= sampleCount - 1;

    yCoCgPixelColorMean[1] = 0.5f + yCoCgPixelColorMean[1];
    yCoCgPixelColorMean[2] = 0.5f + yCoCgPixelColorMean[2];

    for (size_t i = 0; i < 3; ++i) {
      meanTextureData[textureDataOffset + i] = static_cast<uint8_t>(yCoCgPixelColorMean[i] * 255.0f);
      stdDevTextureData[textureDataOffset + i] = static_cast<uint8_t>(
          1.0f / std::max(std::sqrt(covarianceMatrix[i][i]), 1.0f / 255.0f));
    }

    textureDataOffset += 3;
  }

  // Prepare a texture image storing mean color channel values
  glActiveTexture(GL_TEXTURE1);
  glGenTextures(1, &mMeanTexture);
  glBindTexture(GL_TEXTURE_2D, mMeanTexture);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mFrameProperties.dimensions().first,
               mFrameProperties.dimensions().second, 0, GL_RGB,
               GL_UNSIGNED_BYTE, meanTextureData);
  assertNoGlError();

  // Prepare a texture image storing standard deviations of color
  // channel values
  glActiveTexture(GL_TEXTURE1);
  glGenTextures(1, &mStdDevTexture);
  glBindTexture(GL_TEXTURE_2D, mStdDevTexture);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mFrameProperties.dimensions().first,
               mFrameProperties.dimensions().second, 0, GL_RGB,
               GL_UNSIGNED_BYTE, stdDevTextureData);
  assertNoGlError();

  for (auto sampleData : mBackgroundSamples)
    delete[] sampleData;

  mBackgroundSamples.clear();
  mResultSet["foreground_texture"] = mResultTexture;
}


const ProcessingResultSet& NormDistBgSubProcessor::process(GLuint frameTexture) {
  if (mMeanTexture == 0)
    setupBackgroundModel();

  glEnableVertexAttribArray(VertexAttributeLocations::kPosition);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, frameTexture);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, mMeanTexture);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, mStdDevTexture);

  glBindFramebuffer(GL_FRAMEBUFFER, mResultFbo);
  glViewport(0, 0, mFrameProperties.dimensions().first,
             mFrameProperties.dimensions().second);
  glUseProgram(mGlslProgram);

  glClear(GL_COLOR_BUFFER_BIT);
  drawFullscreenQuad(VertexAttributeLocations::kPosition);
  assertNoGlError();

  glDisableVertexAttribArray(VertexAttributeLocations::kPosition);

  return mResultSet;
}


} // end namespace processors
} // end namespace glipf
