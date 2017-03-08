#ifndef glipf_server_handler_h
#define glipf_server_handler_h

#include "thrift-gen-cpp/glipf-server/GlipfServer.h"

#include <glipf/gles-utils/gles-context.h>
#include <glipf/gles-utils/texture-container.h>
#include <glipf/processors/background-subtraction-processor.h>
#include <glipf/processors/foreground-coverage-processor.h>
#include <glipf/processors/foreground-histogram-processor.h>
#include <glipf/processors/model-occlusion-processor.h>
#include <glipf/processors/model-debug-processor.h>
#include <glipf/sinks/display-sink.h>
#include <glipf/sources/frame-source.h>

#include <glm/glm.hpp>

#include <map>


class GlipfServerHandler : public glipf::GlipfServerIf {
public:
  GlipfServerHandler(std::unique_ptr<glipf::sources::FrameSource> frameSource,
                     const glm::mat4& mvpMatrix, float visibilityThreshold);
  void initForegroundCoverageProcessor(const std::vector<glipf::Point3d>& modelCenters,
                                       const glipf::Dims& modelDims) override;
  void scanForeground(std::vector<double>& result) override;
  void initTarget(const glipf::Target& targetData,
                  const bool computeRef) override;
  bool isVisible(const std::vector<glipf::Target>& targets,
                 const glipf::Target& newTarget) override;
  void targetUpdate(const std::vector<glipf::Target>& targets) override;
  void computeDistance(std::vector<double>& result,
                       const std::vector<glipf::Particle>& particles) override;
  void drawDebugOutput(const std::vector<glipf::Target>& targets,
                       const bool drawParticles) override;
  void grabFrame() override;

private:
  double computeBhattDist(const std::vector<float>& refHist,
                          const std::vector<float>& hist);

  glipf::gles_utils::GlesContext mGlesContext;
  glm::mat4 mProjectionMatrix;
  std::unique_ptr<glipf::sources::FrameSource> mFrameSource;
  float mVisibilityThreshold;
  std::unique_ptr<glipf::processors::ModelOcclusionProcessor> mModelOcclusionProcessor;
  std::unique_ptr<glipf::processors::ModelDebugProcessor> mModelDebugProcessor;
  std::unique_ptr<glipf::processors::ForegroundCoverageProcessor> mForegroundCoverageProcessor;
  std::unique_ptr<glipf::processors::ForegroundHistogramProcessor> mForegroundHistogramProcessor;
  std::unique_ptr<glipf::processors::BackgroundSubtractionProcessor> mBackgroundSubtractionProcessor;
  std::unique_ptr<glipf::sinks::DisplaySink> mDisplaySink;
  glipf::Dims mModelDims;
  glipf::gles_utils::TextureContainer mFrameTextureContainer;
  GLuint mForegroundTexture;
  size_t mLastFrameNumber;
  std::map<int32_t, std::vector<float>> mTargetHistograms;
  std::map<int32_t, bool> mTargetOcclusionMap;
  std::map<int32_t, float> mTargetCoverage;
  std::vector<glipf::Particle> mLastParticles;
};

#endif // glipf_server_handler_h
