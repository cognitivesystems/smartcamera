#ifndef background_subtraction_processor_h
#define background_subtraction_processor_h

#include "gles-processor.h"


namespace glipf {
namespace processors {

class BackgroundSubtractionProcessor : public GlesProcessor {
public:
  BackgroundSubtractionProcessor(const sources::FrameProperties& frameProperties,
                                 const void* referenceFrameData);
  ~BackgroundSubtractionProcessor() override;

  virtual const ProcessingResultSet& process(GLuint frameTexture) override;

protected:
  void setupResultFbo();

  GLuint mGlslProgram;
  GLuint mReferenceFrameTexture;
  GLuint mResultTexture;
  GLuint mResultFbo;
};

} // end namespace processors
} // end namespace glipf

#endif // background_subtraction_processor_h
