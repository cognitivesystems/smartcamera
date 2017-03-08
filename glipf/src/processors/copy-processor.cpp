#include <glipf/processors/copy-processor.h>


namespace glipf {
namespace processors {


CopyProcessor::CopyProcessor(const sources::FrameProperties& frameProperties)
  : GlesProcessor(frameProperties)
{
}


const ProcessingResultSet& CopyProcessor::process(GLuint frameTexture) {
  mResultSet["original_frame"] = frameTexture;

  return mResultSet;
}


} // end namespace processors
} // end namespace glipf
