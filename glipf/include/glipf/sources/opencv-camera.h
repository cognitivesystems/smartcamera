#ifndef sources_opencv_camera_h
#define sources_opencv_camera_h

#include "frame-source.h"

#include <opencv2/opencv.hpp>


namespace glipf {
namespace sources {

/**
 * @brief Frame source that acquires frames from the default camera
 * using OpenCV.
 */
class OpenCvCamera : public FrameSource {
public:
  /**
   * @brief Create a new instance of OpenCvCamera.
   *
   * It will acquire frames from the default camera, i.e. `/dev/video0`.
   */
  OpenCvCamera();

  virtual const FrameProperties& getFrameProperties() const override;
  const uint8_t* grabFrame() override;

protected:
  /// Reference to the data source used to acquire frames
  cv::VideoCapture mCaptureReference;
  /// Matrix storing the data of the last captured frame
  cv::Mat mFrameReference;
  FrameProperties mFrameProperties;
};

} // end namespace sources
} // end namespace glipf

#endif // sources_opencv_camera_h
