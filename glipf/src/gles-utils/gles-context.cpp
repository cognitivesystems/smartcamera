#include <glipf/gles-utils/gles-context.h>

#include <GLES2/gl2.h>


#define assertNoGlError() assert(glGetError() == GL_NO_ERROR)
#define UNUSED(x) ((void)x)


namespace glipf {
namespace gles_utils {


GlesContext::GlesContext() {
  // Get an EGL display connection
  mDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  assert(mDisplay != EGL_NO_DISPLAY);

  // Initialize the EGL display connection
  EGLBoolean result = eglInitialize(mDisplay, NULL, NULL);
  assert(result != EGL_FALSE);
  UNUSED(result);

  // Get an appropriate EGL frame buffer configuration
  EGLConfig config;
  EGLint configCount;
  const EGLint attributeList[] = {
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_NONE
  };

  result = eglChooseConfig(mDisplay, attributeList, &config, 1, &configCount);
  assert(result != EGL_FALSE);

  // Set the current rendering API
  result = eglBindAPI(EGL_OPENGL_ES_API);
  assert(result != EGL_FALSE);

  // Create an EGL rendering context
  const EGLint contextAttributes[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };

  mContext = eglCreateContext(mDisplay, config, EGL_NO_CONTEXT,
                              contextAttributes);
  assert(mContext != EGL_NO_CONTEXT);

  // Create a native window
  uint32_t screenWidth, screenHeight;
  int32_t success = graphics_get_display_size(0 /* LCD */, &screenWidth,
                                              &screenHeight);
  assert(success >= 0);
  UNUSED(success);

  mDimensions = std::make_pair(screenWidth, screenHeight);

  VC_RECT_T dstRect;
  dstRect.x = 0;
  dstRect.y = 0;
  dstRect.width = screenWidth;
  dstRect.height = screenHeight;

  VC_RECT_T srcRect;
  srcRect.x = 0;
  srcRect.y = 0;
  srcRect.width = screenWidth << 16;
  srcRect.height = screenHeight << 16;

  DISPMANX_DISPLAY_HANDLE_T dispmanDisplay =
      vc_dispmanx_display_open(0 /* LCD */);
  DISPMANX_UPDATE_HANDLE_T dispmanUpdate = vc_dispmanx_update_start(0);

  DISPMANX_ELEMENT_HANDLE_T dispmanElement =
      vc_dispmanx_element_add(dispmanUpdate, dispmanDisplay, 0 /*layer*/,
                              &dstRect, 0 /*src*/, &srcRect,
                              DISPMANX_PROTECTION_NONE, 0 /*alpha*/,
                              0 /*clamp*/, DISPMANX_NO_ROTATE /*transform*/);

  mNativeWindow.element = dispmanElement;
  mNativeWindow.width = screenWidth;
  mNativeWindow.height = screenHeight;
  vc_dispmanx_update_submit_sync(dispmanUpdate);

  // Create a new EGL window surface
  mSurface = eglCreateWindowSurface(mDisplay, config, &mNativeWindow, NULL);
  assert(mSurface != EGL_NO_SURFACE);

  // Connect the context to the surface
  result = eglMakeCurrent(mDisplay, mSurface, mSurface, mContext);
  assert(result != EGL_FALSE);

  // Adjust common GLES settings
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  assertNoGlError();
}


GlesContext::Dimensions GlesContext::nativeWindowDimensions() const {
  return mDimensions;
}


bool GlesContext::swapBuffers() {
  EGLBoolean result = eglSwapBuffers(mDisplay, mSurface);
  assertNoGlError();

  return result == EGL_TRUE;
}


} // end namespace gles_utils
} // end namespace glipf
