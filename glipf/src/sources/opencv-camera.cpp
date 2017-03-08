#include <glipf/sources/opencv-camera.h>

#include <iostream>


namespace glipf {
namespace sources {


OpenCvCamera::OpenCvCamera()
  : mCaptureReference(0)
  , mFrameProperties(std::make_pair(640, 480), ColorSpace::BGR)
{
  if (!mCaptureReference.isOpened())
    throw FrameSourceInitializationError("Could not open camera");

  mCaptureReference.set(CV_CAP_PROP_FRAME_WIDTH, 640);
  mCaptureReference.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
}


const FrameProperties& OpenCvCamera::getFrameProperties() const {
  return mFrameProperties;
}


const uint8_t* OpenCvCamera::grabFrame() {
  mCaptureReference >> mFrameReference;

  if (mFrameReference.empty()) {
      std::cerr << "Empty frame retrieved!" << std::endl;
      return 0;
  }

  return mFrameReference.data;
}


} // end namespace sources
} // end namespace glipf
