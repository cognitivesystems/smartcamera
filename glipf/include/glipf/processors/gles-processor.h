#ifndef gles_processor_h
#define gles_processor_h

#include "../sources/frame-properties.h"
#include "processing-result.h"

#include <GLES2/gl2.h>

#include <glm/glm.hpp>

#include <cassert>


#define assertNoGlError() assert(glGetError() == GL_NO_ERROR)


namespace glipf {
namespace processors {

class GlesProcessor {
public:
  using ModelData = std::pair<std::vector<GLfloat>, std::vector<GLushort>>;

  GlesProcessor(const sources::FrameProperties& frameProperties);
  virtual ~GlesProcessor();
  virtual const ProcessingResultSet& process(GLuint frameTexture) = 0;

protected:
  using TextureFboPair = std::pair<GLuint, GLuint>;

  TextureFboPair generateTextureBackedFbo(std::pair<size_t, size_t> dimensions);
  void drawFullscreenQuad(GLuint vertexPositionAttribLoc);
  std::vector<double> computeModelAreas(const std::vector<ModelData>& models,
                                        const glm::mat4& mvpMatrix,
                                        size_t viewportWidth,
                                        size_t viewportHeight);

  GLuint mQuadVertexBuffer;
  ProcessingResultSet mResultSet;
  const sources::FrameProperties& mFrameProperties;
};

} // end namespace processors
} // end namespace glipf

#endif // gles_processor_h
