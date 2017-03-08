#ifndef sources_v4l2_camera_h
#define sources_v4l2_camera_h

#include "frame-source.h"

#include <boost/optional.hpp>


namespace glipf {
namespace sources {

/**
 * @brief Frame source that acquires frames from a camera using
 * Video4Linux2 directly.
 */
class V4L2Camera : public FrameSource {
public:
  /**
   * @brief Create a new instance of V4L2Camera.
   *
   * @param imgDimensions dimensions that captured frames should
   *                      preferably have; this value is considered to
   *                      only be a hint and the actual dimensions may
   *                      be different (@ref getFrameProperties should
   *                      be used to discover them)
   * @param deviceName path of the device node (representing a camera)
   *                   from which frames are to be acquired
   */
  V4L2Camera(std::pair<size_t, size_t> imgDimensions,
             std::string deviceName = "/dev/video0");
  ~V4L2Camera() override;

  virtual const FrameProperties& getFrameProperties() const override;
  const uint8_t* grabFrame() override;

protected:
  /// Pointer to data and its corresponding data length
  using BufferData = std::pair<uint8_t*, size_t>;

  /// File descriptor of the camera device used for capture
  int mCameraFD;
  /// Buffers used to store the data of captured frames
  std::vector<BufferData> mBuffers;
  /// Index of the buffer storing the data of the last captured frame
  boost::optional<uint32_t> mUnmappedBufferIndex;
  FrameProperties mFrameProperties;
};

} // end namespace sources
} // end namespace glipf

#endif // sources_v4l2_camera_h
