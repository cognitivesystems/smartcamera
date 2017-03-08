#include <glipf/processors/gles-processor.h>

#include <opencv2/imgproc/imgproc.hpp>


using std::vector;


namespace glipf {
namespace processors {


GlesProcessor::GlesProcessor(const sources::FrameProperties& frameProperties)
  : mQuadVertexBuffer(0)
  , mFrameProperties(frameProperties)
{
  const GLfloat vertex_data[] = {
    -1.0, -1.0, 1.0, 1.0,
    1.0, -1.0, 1.0, 1.0,
    1.0, 1.0, 1.0, 1.0,
    -1.0, 1.0, 1.0, 1.0
  };

  glGenBuffers(1, &mQuadVertexBuffer);
  assertNoGlError();

  // Upload vertex data to a buffer
  glBindBuffer(GL_ARRAY_BUFFER, mQuadVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data,
               GL_STATIC_DRAW);
  assertNoGlError();
}


GlesProcessor::~GlesProcessor() {
  glDeleteBuffers(1, &mQuadVertexBuffer);
}


GlesProcessor::TextureFboPair
GlesProcessor::generateTextureBackedFbo(std::pair<size_t, size_t> dimensions) {
  // Prepare a texture
  GLuint texture;
  glActiveTexture(GL_TEXTURE3);
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dimensions.first, dimensions.second,
               0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
  assertNoGlError();

  // Prepare an FBO
  GLuint fbo;
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         texture, 0);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  assertNoGlError();

  return std::make_pair(texture, fbo);
}


vector<double> GlesProcessor::computeModelAreas(const std::vector<ModelData>& models,
                                                const glm::mat4& mvpMatrix,
                                                size_t viewportWidth,
                                                size_t viewportHeight)
{
  glm::vec2 frameDimensions(mFrameProperties.dimensions().first,
                            mFrameProperties.dimensions().second);
  glm::vec2 viewportDimensions(viewportWidth, viewportHeight);
  vector<double> modelAreas;

  for (auto& model : models) {
    vector<cv::Point> points;

    for (size_t i = 0; i < model.first.size(); i += 3) {
      glm::vec4 vertexVector(model.first[i], model.first[i + 1],
                             model.first[i + 2], 1.0);
      glm::vec4 projectedPosition = mvpMatrix * vertexVector;
      glm::vec2 normalizedPosition(projectedPosition.x / projectedPosition.z,
                                   projectedPosition.y / projectedPosition.z);
      normalizedPosition /= frameDimensions;
      normalizedPosition *= viewportDimensions;

      points.emplace_back(normalizedPosition.x, normalizedPosition.y);
    }

    cv::RotatedRect box = cv::minAreaRect(points);
    modelAreas.push_back(box.size.width * box.size.height);
  }

  return modelAreas;
}


void GlesProcessor::drawFullscreenQuad(GLuint vertexPositionAttribLoc) {
  glBindBuffer(GL_ARRAY_BUFFER, mQuadVertexBuffer);
  glVertexAttribPointer(vertexPositionAttribLoc, 4, GL_FLOAT, GL_FALSE, 0, 0);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  assertNoGlError();
}


} // end namespace processors
} // end namespace glipf
