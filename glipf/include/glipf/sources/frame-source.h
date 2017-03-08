#ifndef sources_frame_source_h
#define sources_frame_source_h

#include "frame-properties.h"

#include <cstdint>
#include <stdexcept>


namespace glipf {
namespace sources {


/// Exception raised when a frame source fails to initialise.
class FrameSourceInitializationError : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};


/// Abstract base class defining the common API of all frame sources.
class FrameSource {
public:
  virtual ~FrameSource() {};

  /// Return the properties of frames produced by the frame source.
  virtual const FrameProperties& getFrameProperties() const = 0;
  /**
   * Capture a frame and return its data.
   *
   * \note The number of bytes returned depends on the properties of the
   *       frame (as reported by @ref getFrameProperties) and can be
   *       calculated as follows:
   * ~~~
   * // frameSource is an instance of a concrete subclass of FrameSource
   * std::pair<size_t, size_t> frameDimensions = frameSource.getFrameProperties.dimensions();
   * size_t pixelCount = frameDimensions.first * frameDimensions.second;
   * size_t byteCount = pixelCount * 3;
   * ~~~
   */
  virtual const uint8_t* grabFrame() = 0;
};


} // end namespace sources
} // end namespace glipf

#endif // sources_frame_source_h
