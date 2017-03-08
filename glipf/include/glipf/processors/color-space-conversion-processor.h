#ifndef color_space_conversion_processor_h
#define color_space_conversion_processor_h

#include "gles-processor.h"


namespace glipf {
namespace processors {

class ColorSpaceConversionProcessor : public GlesProcessor {
public:
  ColorSpaceConversionProcessor(const sources::FrameProperties& frameProperties,
                                sources::ColorSpace from,
                                sources::ColorSpace to);
  ~ColorSpaceConversionProcessor();

  virtual const ProcessingResultSet& process(GLuint frameTexture) override;

protected:
  GLuint mGlslProgram;
  GLuint mResultTexture;
  GLuint mResultFbo;
};

} // end namespace processors
} // end namespace glipf

#endif // color_space_conversion_processor_h
