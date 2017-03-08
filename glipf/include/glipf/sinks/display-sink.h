#ifndef sinks_display_sink_h
#define sinks_display_sink_h

#include "sink.h"


namespace glipf {
namespace sinks {

class DisplaySink : public Sink {
public:
  DisplaySink(uint32_t width, uint32_t height);

  virtual void send(const processors::ProcessingResultSet& resultSet) override;

protected:
  void drawFullscreenQuad();
  void displayTextures(size_t textureCount,
                       const processors::ProcessingResultSet& resultSet);

  uint32_t mWidth;
  uint32_t mHeight;
  GLuint mQuadVertexBuffer;
  GLuint mGlslProgram;
};

} // end namespace sinks
} // end namespace glipf

#endif // sinks_display_sink_h
