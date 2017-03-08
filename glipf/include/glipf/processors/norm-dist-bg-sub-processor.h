#ifndef processors_norm_dist_bg_sub_processor_h
#define processors_norm_dist_bg_sub_processor_h

#include "gles-processor.h"


namespace glipf {
namespace processors {

class NormDistBgSubProcessor : public GlesProcessor {
public:
  NormDistBgSubProcessor(const sources::FrameProperties& frameProperties);
  ~NormDistBgSubProcessor() override;

  virtual const ProcessingResultSet& process(GLuint frameTexture) override;
  void addBackgroundSample(const void* frameData);

protected:
  void setupResultFbo();
  void setupBackgroundModel();

  std::vector<uint8_t*> mBackgroundSamples;
  GLuint mGlslProgram;
  GLuint mReferenceFrameTexture;
  GLuint mMeanTexture;
  GLuint mStdDevTexture;
  GLuint mResultTexture;
  GLuint mResultFbo;
};

} // end namespace processors
} // end namespace glipf

#endif // processors_norm_dist_bg_sub_processor_h
