#ifndef model_debug_processor_h
#define model_debug_processor_h

#include "gles-processor.h"

#include <glm/glm.hpp>


namespace glipf {
namespace processors {

class ModelDebugProcessor : public GlesProcessor {
public:
  ModelDebugProcessor(const sources::FrameProperties& frameProperties,
                      const glm::mat4& mvpMatrix);
  ~ModelDebugProcessor() override;

  void setModels(const std::vector<ModelData>& models,
                 const std::vector<uint_fast8_t>& modelGroups = std::vector<uint_fast8_t>());
  virtual const ProcessingResultSet& process(GLuint frameTexture) override;

protected:
  using TextureFboPair = std::pair<GLuint, GLuint>;

  glm::vec4 getGroupColor(uint_fast8_t groupId);

  GLuint mPassthroughGlslProgram;
  GLuint mMainGlslProgram;
  size_t mModelIndexCount;
  GLuint mModelVertexBuffer;
  GLuint mModelIndexBuffer;
  TextureFboPair mTextureFboPair;
};

} // end namespace processors
} // end namespace glipf

#endif // model_debug_processor_h
