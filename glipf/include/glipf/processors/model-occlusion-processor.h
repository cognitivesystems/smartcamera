#ifndef model_occlusion_processor_h
#define model_occlusion_processor_h

#include "gles-processor.h"

#include <glm/glm.hpp>


namespace glipf {
namespace processors {

class ModelOcclusionProcessor : public GlesProcessor {
public:
  using ModelData = std::pair<std::vector<GLfloat>, std::vector<GLushort>>;

  ModelOcclusionProcessor(const sources::FrameProperties& frameProperties,
                          const glm::mat4& mvpMatrix);
  ~ModelOcclusionProcessor() override;

  void setModels(const std::vector<ModelData>& models,
                 const glm::mat4& mvpMatrix);
  virtual const ProcessingResultSet& process(GLuint frameTexture) override;

protected:
  void setupModelGeometry(const std::vector<ModelData>& models);

  size_t mModelCount;
  std::vector<double> mModelAreas;
  GLuint mMainGlslProgram;
  size_t mModelIndexCount;
  GLuint mModelVertexBuffer;
  GLuint mModelIndexBuffer;
  GLuint mTexture;
  GLuint mRenderBuffer;
  GLuint mFrameBuffer;
};

} // end namespace processors
} // end namespace glipf

#endif // model_occlusion_processor_h
