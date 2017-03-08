#ifndef gles_utils_gles_context_h
#define gles_utils_gles_context_h

#include <bcm_host.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <cassert>
#include <utility>


namespace glipf {
namespace gles_utils {

class GlesContext {
public:
  using Dimensions = std::pair<uint_fast16_t, uint_fast16_t>;

  GlesContext();

  Dimensions nativeWindowDimensions() const;
  bool swapBuffers();

protected:
  Dimensions mDimensions;
  EGLDisplay mDisplay;
  EGL_DISPMANX_WINDOW_T mNativeWindow;
  EGLSurface mSurface;
  EGLContext mContext;
};

} // end namespace gles_utils
} // end namespace glipf

#endif // gles_utils_gles_context_h
