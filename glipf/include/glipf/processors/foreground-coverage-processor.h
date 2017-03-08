#ifndef foreground_coverage_processor_h
#define foreground_coverage_processor_h

#include "gles-processor.h"

#include <glm/glm.hpp>


namespace glipf {
namespace processors {

class ForegroundCoverageProcessor : public GlesProcessor {
public:
  ForegroundCoverageProcessor(const sources::FrameProperties& frameProperties,
                              const std::vector<ModelData>& models,
                              const glm::mat4& mvpMatrix);
  ~ForegroundCoverageProcessor();

  virtual const ProcessingResultSet& process(GLuint frameTexture) override;

protected:
  using TextureFboPair = std::pair<GLuint, GLuint>;
  using ReductionFboSet = std::tuple<size_t, GLuint, GLuint,
                                     std::vector<TextureFboPair>>;
  using ReductionFboSpec = std::tuple<GLuint, uint_fast16_t, uint_fast16_t>;

  void setupModelGeometry(const std::vector<ModelData>& models);
  void addReductionFboSet(size_t modelCount, GLuint indexOffset,
                          GLuint indexCount);
  void setupReductionGlslPrograms(const glm::mat4& mvpMatrix);
  void startForegroundCoverage(void* referenceFrameData);
  void calculateForegroundCoverage();

  GLuint mPixelCountingGlslProgram;
  GLuint mModelVertexBuffer;
  GLuint mModelIndexBuffer;
  std::vector<ReductionFboSet> mReductionFboSets;
  std::vector<ReductionFboSpec> mReductionFboSpecs;
};

} // end namespace processors
} // end namespace glipf

#endif // foreground_coverage_processor_h
