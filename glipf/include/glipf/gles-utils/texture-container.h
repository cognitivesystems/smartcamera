#ifndef gles_utils_texture_container_h
#define gles_utils_texture_container_h

#include <GLES2/gl2.h>

#include <cstddef>
#include <utility>


namespace glipf {
namespace gles_utils {

class TextureContainer {
public:
  TextureContainer(std::pair<size_t, size_t> dimensions);
  ~TextureContainer();

  void uploadData(const void* frameData);
  GLuint getTexture() const;

protected:
  std::pair<size_t, size_t> mDimensions;
  GLuint mTexture;
};

} // end namespace gles_utils
} // end namespace glipf

#endif // gles_utils_texture_container_h
