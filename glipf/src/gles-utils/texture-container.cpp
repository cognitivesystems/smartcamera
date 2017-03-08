#include <glipf/gles-utils/texture-container.h>

#include <cassert>


#define assertNoGlError() assert(glGetError() == GL_NO_ERROR)


namespace glipf {
namespace gles_utils {


TextureContainer::TextureContainer(std::pair<size_t, size_t> dimensions)
  : mDimensions(dimensions)
{
  glGenTextures(1, &mTexture);
  glBindTexture(GL_TEXTURE_2D, mTexture);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  assertNoGlError();
}


TextureContainer::~TextureContainer() {
  glDeleteTextures(1, &mTexture);
}


void TextureContainer::uploadData(const void* frameData) {
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, mTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mDimensions.first, mDimensions.second,
               0, GL_RGB, GL_UNSIGNED_BYTE, frameData);
  assertNoGlError();
}


GLuint TextureContainer::getTexture() const {
  return mTexture;
}


} // end namespace gles_utils
} // end namespace glipf
