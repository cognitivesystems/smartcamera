#include <glipf/sources/opencv-video-source.h>

#include <iostream>


namespace glipf {
namespace sources {


OpenCvVideoSource::OpenCvVideoSource(const std::string& filePath)
  : mCaptureReference(filePath)
{
  if (!mCaptureReference.isOpened()) {
      throw FrameSourceInitializationError("Could not open video `" +
                                           filePath + "`");
  }

  size_t frameWidth = mCaptureReference.get(CV_CAP_PROP_FRAME_WIDTH);
  size_t frameHeight = mCaptureReference.get(CV_CAP_PROP_FRAME_HEIGHT);
  mFrameDimensions = std::make_pair(frameWidth, frameHeight);

  mFrameProperties.reset(new FrameProperties(std::make_pair(frameWidth,
                                                            frameHeight),
                                             ColorSpace::BGR));
}


const FrameProperties& OpenCvVideoSource::getFrameProperties() const {
  return *mFrameProperties;
}


const uint8_t* OpenCvVideoSource::grabFrame() {
  mCaptureReference >> mFrameReference;

  if (mFrameReference.empty()) {
      std::cerr << "Empty frame retrieved!" << std::endl;
      return 0;
  }

  return mFrameReference.data;
}


} // end namespace sources
} // end namespace glipf
