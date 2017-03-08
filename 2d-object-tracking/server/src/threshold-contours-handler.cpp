#include "threshold-contours-handler.h"

#include <glipf/sources/frame-source.h>

#include <boost/variant/get.hpp>
#include <opencv2/opencv.hpp>


using glipf::processors::ForegroundHistogramProcessor;
using glipf::processors::GlesProcessor;
using glipf::processors::ModelDebugProcessor;
using glipf::processors::ProcessingResultSet;
using glipf::sinks::DisplaySink;
using glipf::sources::FrameSource;

using std::unique_ptr;
using std::vector;


GlesProcessor::ModelData generateRectModel(const cv::Rect& rect) {
  float x = rect.x, y = rect.y;

  return {{
    x, y, 1.0f,
    x, y + rect.height, 1.0f,
    x + rect.width, y, 1.0f,
    x + rect.width, y + rect.height, 1.0f
  }, {
    0, 1, 2,
    3, 1, 2
  }};
}


GlesProcessor::ModelData generateWireframeModel(const cv::Rect& rect) {
  float x = rect.x, y = rect.y;

  return {{
    x, y, 1.0f,
    x, y + rect.height, 1.0f,
    x + rect.width, y, 1.0f,
    x + rect.width, y + rect.height, 1.0f
  }, {
    0, 1,
    1, 3,
    2, 3,
    0, 2,
    0, 3,
    1, 2
  }};
}


ThresholdContoursHandler::ThresholdContoursHandler(unique_ptr<FrameSource> frameSource,
                                                   const glm::mat4& mvpMatrix)
  : mProjectionMatrix(mvpMatrix)
  , mFrameTextureContainer(frameSource->getFrameProperties().dimensions())
  , mThresholdedTexture(0)
  , mFrameSource(std::move(frameSource))
{
}

void ThresholdContoursHandler::initThresholdProcessor(const vector<glipf::Threshold>& thresholds) {
  // Drop the first batch of frames to let the camera image settle
  for (size_t i = 0; i < 15; ++i)
    mFrameSource->grabFrame();

  for (auto& threshold : thresholds) {
    glm::vec3 lowerThreshold(threshold.lower.h, threshold.lower.s,
                             threshold.lower.v);
    glm::vec3 upperThreshold(threshold.upper.h, threshold.upper.s,
                             threshold.upper.v);
    mThresholdProcessors.emplace_back(mFrameSource->getFrameProperties(),
                                      lowerThreshold, upperThreshold);
  }

  mDisplaySink.reset(new DisplaySink(mGlesContext.nativeWindowDimensions().first,
                                     mGlesContext.nativeWindowDimensions().second));
  mModelDebugProcessor.reset(new ModelDebugProcessor(mFrameSource->getFrameProperties(),
                                                     mProjectionMatrix));
  mForegroundHistogramProcessor.reset(
      new ForegroundHistogramProcessor(mFrameSource->getFrameProperties(), 96,
                                       mProjectionMatrix));
}


void ThresholdContoursHandler::getThresholdRects(vector<vector<glipf::Rect> >& result) {
  mFrameTextureContainer.uploadData(mFrameSource->grabFrame());

  ProcessingResultSet combinedResultSet;
  vector<GlesProcessor::ModelData> models;

  for (auto& processor : mThresholdProcessors) {
    const auto& resultSet =
        processor.process(mFrameTextureContainer.getTexture());
    combinedResultSet.insert(std::begin(resultSet), std::end(resultSet));
    mThresholdedTexture = boost::get<GLuint>(resultSet.at("thresholded_texture"));

    size_t fboWidth = mFrameSource->getFrameProperties().dimensions().first;
    size_t fboHeight = mFrameSource->getFrameProperties().dimensions().second;

    GLubyte pixelData[fboWidth * fboHeight * 4];
    glReadPixels(0, 0, fboWidth, fboHeight, GL_RGBA, GL_UNSIGNED_BYTE,
                 pixelData);

    cv::Mat image(fboHeight, fboWidth, CV_8UC4, pixelData);
    cv::Mat alpha(image.rows, image.cols, CV_8UC1);
    int channelMapping[] = { 3, 0 };
    mixChannels(&image, 1, &alpha, 1, channelMapping, 1);

    vector< vector<cv::Point> > contours;
    vector<cv::Vec4i> hierarchy;
    findContours(alpha, contours, hierarchy, CV_RETR_EXTERNAL,
                 CV_CHAIN_APPROX_SIMPLE);

    result.emplace_back();
    vector<glipf::Rect>& thresholdResult = result.back();

    for (auto& points : contours) {
      double contArea = contourArea(points);

      if (contArea < 1000.0)
        continue;

      cv::Rect rect = boundingRect(points);
      models.push_back(generateRectModel(rect));

      thresholdResult.emplace_back();
      glipf::Rect& resultRect = thresholdResult.back();
      resultRect.x = rect.x;
      resultRect.y = rect.y;
      resultRect.w = rect.width;
      resultRect.h = rect.height;
    }
  }

  mModelDebugProcessor->setModels(models);
  const auto& resultSet =
      mModelDebugProcessor->process(mFrameTextureContainer.getTexture());
  combinedResultSet.insert(std::begin(resultSet), std::end(resultSet));

  mDisplaySink->send(combinedResultSet);
  mGlesContext.swapBuffers();
}


void ThresholdContoursHandler::initTarget(const glipf::Target& targetData) {
  size_t fboWidth = mFrameSource->getFrameProperties().dimensions().first;
  size_t fboHeight = mFrameSource->getFrameProperties().dimensions().second;

  std::vector<GlesProcessor::ModelData> models = {
    generateRectModel(cv::Rect(targetData.pose.x, targetData.pose.y,
                               targetData.pose.w, targetData.pose.h))
  };

  mForegroundHistogramProcessor->setModels(models, mProjectionMatrix);
  const auto& resultSet =
      mForegroundHistogramProcessor->process(mThresholdedTexture);

  mTargetHistograms[targetData.id] =
      boost::get<vector<float>>(resultSet.at("0"));
  float pixelCount =
      boost::get<vector<float>>(resultSet.at("total_pixel_counts"))[0];
  float targetArea = targetData.pose.w * targetData.pose.h;
  mTargetCoverage[targetData.id] = pixelCount / targetArea;

  std::vector<GlesProcessor::ModelData> debugModels = {
    generateWireframeModel(cv::Rect(targetData.pose.x, targetData.pose.y,
                                    targetData.pose.w, targetData.pose.h))
  };

  mModelDebugProcessor->setModels(debugModels);
  mModelDebugProcessor->process(mFrameTextureContainer.getTexture());

  GLubyte pixelData[fboWidth * fboHeight * 4];
  glReadPixels(0, 0, fboWidth, fboHeight, GL_RGBA, GL_UNSIGNED_BYTE,
               pixelData);

  cv::Mat image(fboHeight, fboWidth, CV_8UC4, pixelData);
  imwrite("target-detection-debug" + std::to_string(0) + ".png",
          image);
}


double ThresholdContoursHandler::computeBhattDist(const vector<float>& refHist,
                                                  const vector<float>& hist)
{
  // Compute bhattacharya distance between hist and refHist
  double bhattDist = 0.0;
  auto refHistIter = std::begin(refHist);

  for (auto value : hist) {
    auto refValue = *(refHistIter++);

    if (value != 0 && refValue != 0)
      bhattDist += std::sqrt(value * refValue);
  }

  return bhattDist;
}


void ThresholdContoursHandler::computeDistance(vector<double>& result,
                                               const vector<glipf::Target>& targets,
                                               const vector<glipf::Particle>& particles)
{
  mFrameTextureContainer.uploadData(mFrameSource->grabFrame());
  ProcessingResultSet combinedResultSet;

  for (auto& processor : mThresholdProcessors) {
    const auto& resultSet =
        processor.process(mFrameTextureContainer.getTexture());
    mThresholdedTexture = boost::get<GLuint>(resultSet.at("thresholded_texture"));
    combinedResultSet.insert(std::begin(resultSet), std::end(resultSet));
  }

  vector<GlesProcessor::ModelData> models;
  vector<uint_fast8_t> modelGroups;

  for (auto& particle : particles) {
    models.push_back(generateRectModel(cv::Rect(particle.pose.x,
                                                particle.pose.y,
                                                particle.pose.w,
                                                particle.pose.h)));
    modelGroups.push_back(1 + particle.id);
  }

  mForegroundHistogramProcessor->setModels(models, mProjectionMatrix);
  const auto& resultSet = mForegroundHistogramProcessor->process(mThresholdedTexture);
  const auto& totalPixelCounts =
      boost::get<vector<float>>(resultSet.at("total_pixel_counts"));
  size_t i = 0;

  for (auto& particle : particles) {
    float pixelCount = totalPixelCounts[i];

    if (pixelCount < 1.0f) {
      result.push_back(-1.0);
      continue;
    }

    float targetArea = particle.pose.w * particle.pose.h;
    float particleCoverage = pixelCount / targetArea;
    float coverageDiffPercentage = std::abs(
        1 - particleCoverage / mTargetCoverage[particle.id]);

    auto histKey = std::to_string(i++);
    auto& refHist = mTargetHistograms.at(particle.id);
    auto& hist = boost::get<vector<float>>(resultSet.at(histKey));
    auto bhattDist = computeBhattDist(refHist, hist);
    bhattDist *= 1 - coverageDiffPercentage;

    if (bhattDist <= 0)
      result.push_back(-1.0);
    else
      result.push_back(std::sqrt(1.0 - std::sqrt(bhattDist)));
  }

  for (auto& target : targets) {
    models.push_back(generateWireframeModel(cv::Rect(target.pose.x,
                                                     target.pose.y,
                                                     target.pose.w,
                                                     target.pose.h)));
    modelGroups.push_back(0);
  }

  mModelDebugProcessor->setModels(models, modelGroups);
  const auto& debugResultSet =
      mModelDebugProcessor->process(mFrameTextureContainer.getTexture());
  combinedResultSet.insert(std::begin(debugResultSet),
                           std::end(debugResultSet));

  mDisplaySink->send(combinedResultSet);
  mGlesContext.swapBuffers();
}
