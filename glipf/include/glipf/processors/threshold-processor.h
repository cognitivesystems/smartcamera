#ifndef threshold_processor_h
#define threshold_processor_h

#include "gles-processor.h"


namespace glipf {
namespace processors {

class ThresholdProcessor : public GlesProcessor {
public:
  ThresholdProcessor(const sources::FrameProperties& frameProperties,
                     glm::vec3 lowerHsvThreshold, glm::vec3 upperHsvThreshold);
  ~ThresholdProcessor();

  virtual const ProcessingResultSet& process(GLuint frameTexture) override;

protected:
  GLuint mGlslProgram;
  GLuint mResultTexture;
  GLuint mResultFbo;
};

} // end namespace processors
} // end namespace glipf

#endif // threshold_processor_h
