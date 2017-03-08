#ifndef gles_utils_dump_to_image_h
#define gles_utils_dump_to_image_h

#include <GLES2/gl2.h>

#include <string>


namespace glipf {
namespace gles_utils {

void dumpFramebufferToImage(GLuint framebuffer,
                            std::pair<size_t, size_t> dimensions,
                            const std::string& filename);

void dumpTextureToImage(GLuint texture, std::pair<size_t, size_t> dimensions,
                        const std::string& filename);

} // end namespace gles_utils
} // end namespace glipf

#endif // gles_utils_dump_to_image_h
