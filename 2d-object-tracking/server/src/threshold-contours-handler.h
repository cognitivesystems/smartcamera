#ifndef handlers_threshold_contours_handler_h
#define handlers_threshold_contours_handler_h

#include <glipf/gles-utils/gles-context.h>
#include <glipf/gles-utils/texture-container.h>
#include <glipf/processors/foreground-histogram-processor.h>
#include <glipf/processors/model-debug-processor.h>
#include <glipf/processors/threshold-processor.h>
#include <glipf/sinks/display-sink.h>
#include <glipf/sources/frame-source.h>
#include "thrift-gen-cpp/threshold-contours/ThresholdContours.h"



class ThresholdContoursHandler : virtual public glipf::ThresholdContoursIf {
public:
  ThresholdContoursHandler(std::unique_ptr<glipf::sources::FrameSource> frameSource,
                           const glm::mat4& mvpMatrix);

  void initThresholdProcessor(const std::vector<glipf::Threshold>& thresholds) override;
  void getThresholdRects(std::vector<std::vector<glipf::Rect> >& result) override;
  void initTarget(const glipf::Target& targetData) override;
  void computeDistance(std::vector<double>& result,
                       const std::vector<glipf::Target>& targets,
                       const std::vector<glipf::Particle>& particles) override;

private:
  double computeBhattDist(const std::vector<float>& refHist,
                          const std::vector<float>& hist);

  glipf::gles_utils::GlesContext mGlesContext;
  glm::mat4 mProjectionMatrix;
  glipf::gles_utils::TextureContainer mFrameTextureContainer;
  GLuint mThresholdedTexture;
  std::unique_ptr<glipf::sources::FrameSource> mFrameSource;
  std::unique_ptr<glipf::sinks::DisplaySink> mDisplaySink;
  std::unique_ptr<glipf::processors::ForegroundHistogramProcessor> mForegroundHistogramProcessor;
  std::unique_ptr<glipf::processors::ModelDebugProcessor> mModelDebugProcessor;
  std::vector<glipf::processors::ThresholdProcessor> mThresholdProcessors;
  std::map<int32_t, std::vector<float>> mTargetHistograms;
  std::map<int32_t, float> mTargetCoverage;
};

#endif // handlers_threshold_contours_handler_h
