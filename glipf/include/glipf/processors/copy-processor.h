#ifndef copy_processor_h
#define copy_processor_h

#include "gles-processor.h"


namespace glipf {
namespace processors {

class CopyProcessor : public GlesProcessor {
public:
  CopyProcessor(const sources::FrameProperties& frameProperties);

  virtual const ProcessingResultSet& process(GLuint frameTexture) override;
};

} // end namespace processors
} // end namespace glipf

#endif // copy_processor_h
