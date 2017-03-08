#ifndef foreground_histogram_processor_h
#define foreground_histogram_processor_h

#include "gles-processor.h"

#include <glm/glm.hpp>


namespace glipf {
namespace processors {

class ForegroundHistogramProcessor : public GlesProcessor {
public:
  using ModelData = std::pair<std::vector<GLfloat>, std::vector<GLushort>>;

  ForegroundHistogramProcessor(const sources::FrameProperties& frameProperties,
                               size_t maxModelCount,
                               const glm::mat4& mvpMatrix);
  ~ForegroundHistogramProcessor() override;
  void setModels(const std::vector<ModelData>& models,
                 const glm::mat4& mvpMatrix);

  virtual const ProcessingResultSet& process(GLuint frameTexture) override;

protected:
  using TextureFboPair = std::pair<GLuint, GLuint>;
  using ReductionFboSet = std::tuple<size_t, GLuint, GLuint>;
  using ReductionFboSpec = std::tuple<GLuint, uint_fast16_t, uint_fast16_t>;
  using HistogramFboSpec = std::pair<GLint, GLsizei>;

  void setupHistogramBuffers(const std::vector<ModelData>& models,
                             const glm::mat4& mvpMatrix);
  void setupFbos(size_t modelCount);
  void setupModelGeometry(const std::vector<ModelData>& models);
  void addModelForegroundFbo();
  void addHistogramFbo();
  void setupReductionGlslPrograms(const glm::mat4& mvpMatrix);

  size_t mModelCount;
  size_t mMaxModelCount;
  std::vector<double> mModelAreas;
  GLuint mModelVertexBuffer;
  GLuint mModelIndexBuffer;
  GLuint mScatterPointsBuffer;
  GLuint mHistogramGlslProgram;
  std::vector<ReductionFboSet> mReductionFboSets;
  std::vector<ReductionFboSpec> mReductionFboSpecs;
  std::vector<GLuint> mForegroundTextures;
  std::vector<GLuint> mForegroundFbos;
  std::vector<GLuint> mHistogramTextures;
  std::vector<GLuint> mHistogramFbos;
  std::vector<HistogramFboSpec> mHistogramFboSpecs;
};

} // end namespace processors
} // end namespace glipf

#endif // foreground_histogram_processor_h
