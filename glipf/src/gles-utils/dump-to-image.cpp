#include <glipf/gles-utils/dump-to-image.h>

#include <opencv2/opencv.hpp>

#include <cassert>


#define assertNoGlError() assert(glGetError() == GL_NO_ERROR)


namespace glipf {
namespace gles_utils {


void dumpFramebufferToImage(GLuint framebuffer,
                            std::pair<size_t, size_t> dimensions,
                            const std::string& filename)
{
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

  GLubyte* pixelData = new GLubyte[dimensions.first * dimensions.second * 4];
  glReadPixels(0, 0, dimensions.first, dimensions.second, GL_RGBA,
               GL_UNSIGNED_BYTE, pixelData);

  cv::Mat image(dimensions.second, dimensions.first, CV_8UC4, pixelData);
  imwrite(filename, image);

  delete[] pixelData;
  assertNoGlError();
}


void dumpTextureToImage(GLuint texture, std::pair<size_t, size_t> dimensions,
                        const std::string& filename)
{
  // Attach the texture to an FBO so its data can be read
  GLuint tempFbo;
  glGenFramebuffers(1, &tempFbo);
  glBindFramebuffer(GL_FRAMEBUFFER, tempFbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         texture, 0);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  assertNoGlError();

  dumpFramebufferToImage(tempFbo, dimensions, filename);
  glDeleteFramebuffers(1, &tempFbo);
}


} // end namespace gles_utils
} // end namespace glipf
