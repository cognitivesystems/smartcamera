#ifndef sources_frame_properties_h
#define sources_frame_properties_h

#include <utility>
#include <cstddef>


namespace glipf {
namespace sources {


enum class ColorSpace {
  RGB, BGR, YUV, HSV
};


/// Properties of frames returned from a data source.
class FrameProperties {
public:
  FrameProperties(std::pair<size_t, size_t> dimensions, ColorSpace colorSpace);

  /// Return the dimensions of frames returned from a data source.
  std::pair<size_t, size_t> dimensions() const;
  /// Return the colour space of frames returned from a data source.
  ColorSpace colorSpace() const;

protected:
  std::pair<size_t, size_t> mDimensions;
  ColorSpace mColorSpace;
};


} // end namespace sources
} // end namespace glipf

#endif // sources_frame_properties_h
