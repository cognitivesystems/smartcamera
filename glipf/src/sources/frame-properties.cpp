#include <glipf/sources/frame-properties.h>


namespace glipf {
namespace sources {


FrameProperties::FrameProperties(std::pair<size_t, size_t> dimensions,
                                 ColorSpace colorSpace)
  : mDimensions(dimensions)
  , mColorSpace(colorSpace)
{}


std::pair<size_t, size_t> FrameProperties::dimensions() const {
  return mDimensions;
}


ColorSpace FrameProperties::colorSpace() const {
  return mColorSpace;
}


} // end namespace sources
} // end namespace glipf
