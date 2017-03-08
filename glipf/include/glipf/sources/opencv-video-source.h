#ifndef sources_opencv_video_source_h
#define sources_opencv_video_source_h

#include "frame-source.h"

#include <opencv2/opencv.hpp>

#include <memory>


namespace glipf {
namespace sources {

/**
 * @brief Frame source that acquires frames from a video file or an
 * image series.
 *
 * The class uses components provided by OpenCV to acquire and return
 * frames.
 */
class OpenCvVideoSource : public FrameSource {
public:
  /**
   * @brief Create a new instance of OpenCvVideoSource.
   *
   * @param filePath path of the video file or image series to read
   *                 frames from
   *
   * Example (open a video file):
   * ~~~
   * OpenCvVideoSource frameSource("video.webm");
   * ~~~
   *
   * Example (open an image series):
   * ~~~
   * OpenCvVideoSource frameSource("image-series/frame%03d.png");
   * ~~~
   */
  OpenCvVideoSource(const std::string& filePath);

  virtual const FrameProperties& getFrameProperties() const override;
  const uint8_t* grabFrame() override;

protected:
  /// Reference to the data source used to acquire frames
  cv::VideoCapture mCaptureReference;
  /// Matrix storing the data of the last captured frame
  cv::Mat mFrameReference;
  std::pair<size_t, size_t> mFrameDimensions;
  std::unique_ptr<FrameProperties> mFrameProperties;
};

} // end namespace sources
} // end namespace glipf

#endif // sources_opencv_video_source_h
