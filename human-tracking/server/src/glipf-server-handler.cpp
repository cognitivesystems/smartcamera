#include "glipf-server-handler.h"

#include <boost/variant/get.hpp>
#include <opencv2/opencv.hpp>


using glipf::processors::BackgroundSubtractionProcessor;
using glipf::processors::ForegroundCoverageProcessor;
using glipf::processors::ForegroundHistogramProcessor;
using glipf::processors::GlesProcessor;
using glipf::processors::ModelDebugProcessor;
using glipf::processors::ModelOcclusionProcessor;
using glipf::sinks::DisplaySink;
using glipf::sources::FrameSource;

using std::vector;
using std::unique_ptr;


GlesProcessor::ModelData generateCuboidData(float cx, float cy, float cz,
                                            const glipf::Dims& modelDims)
{
  float halfWidth = modelDims.x / 2.0f;
  float halfHeight = modelDims.y / 2.0f;
  float halfDepth = modelDims.z / 2.0f;

  return {{
    cx - halfWidth, cy - halfHeight, cz - halfDepth,
    cx - halfWidth, cy - halfHeight, cz + halfDepth,
    cx - halfWidth, cy + halfHeight, cz - halfDepth,
    cx - halfWidth, cy + halfHeight, cz + halfDepth,
    cx + halfWidth, cy - halfHeight, cz - halfDepth,
    cx + halfWidth, cy - halfHeight, cz + halfDepth,
    cx + halfWidth, cy + halfHeight, cz - halfDepth,
    cx + halfWidth, cy + halfHeight, cz + halfDepth
  }, {
    0, 2, 4, 2, 4, 6,
    1, 3, 5, 3, 5, 7,
    0, 1, 4, 1, 4, 5,
    2, 3, 6, 3, 6, 7,
    0, 1, 2, 1, 2, 3,
    4, 5, 6, 5, 6, 7
  }};
}


GlesProcessor::ModelData generateWireframeModel(float cx, float cy, float cz,
                                                const glipf::Dims& modelDims)
{
  float halfWidth = modelDims.x / 2.0f;
  float halfHeight = modelDims.y / 2.0f;
  float halfDepth = modelDims.z / 2.0f;

  return {{
    cx - halfWidth, cy - halfHeight, cz - halfDepth,
    cx - halfWidth, cy - halfHeight, cz + halfDepth,
    cx - halfWidth, cy + halfHeight, cz - halfDepth,
    cx - halfWidth, cy + halfHeight, cz + halfDepth,
    cx + halfWidth, cy - halfHeight, cz - halfDepth,
    cx + halfWidth, cy - halfHeight, cz + halfDepth,
    cx + halfWidth, cy + halfHeight, cz - halfDepth,
    cx + halfWidth, cy + halfHeight, cz + halfDepth
  }, {
    0, 1,
    0, 2,
    1, 3,
    2, 3,
    4, 5,
    4, 6,
    5, 7,
    6, 7,
    0, 4,
    2, 6,
    1, 5,
    3, 7
  }};
}


GlipfServerHandler::GlipfServerHandler(unique_ptr<FrameSource> frameSource,
                                       const glm::mat4& mvpMatrix,
                                       float visibilityThreshold)
  : mProjectionMatrix(mvpMatrix)
  , mFrameSource(std::move(frameSource))
  , mVisibilityThreshold(visibilityThreshold)
  , mFrameTextureContainer(mFrameSource->getFrameProperties().dimensions())
  , mForegroundTexture(0)
  , mLastFrameNumber(0)
{
}


void GlipfServerHandler::grabFrame() {
  mFrameTextureContainer.uploadData(mFrameSource->grabFrame());
  ++mLastFrameNumber;

  const auto& resultSet =
      mBackgroundSubtractionProcessor->process(mFrameTextureContainer.getTexture());
  mForegroundTexture = boost::get<GLuint>(resultSet.at("foreground_texture"));
}


void GlipfServerHandler::initForegroundCoverageProcessor(const vector<glipf::Point3d>& modelCenters,
                                                         const glipf::Dims& modelDims)
{
  std::vector<GlesProcessor::ModelData> models;

  for (auto& modelCenter : modelCenters) {
    models.push_back(generateCuboidData(modelCenter.x, modelCenter.y,
                                        modelCenter.z, modelDims));
  }

  const void* frameData;

  for (size_t i = 0; i < 4; ++i)
    frameData = mFrameSource->grabFrame();

  mModelDims = modelDims;
  mLastFrameNumber = 3;
  mFrameTextureContainer.uploadData(frameData);

  mBackgroundSubtractionProcessor.reset(
      new BackgroundSubtractionProcessor(mFrameSource->getFrameProperties(),
                                         frameData));
  mForegroundCoverageProcessor.reset(
      new ForegroundCoverageProcessor(mFrameSource->getFrameProperties(),
                                      models, mProjectionMatrix));
  mForegroundHistogramProcessor.reset(
      new ForegroundHistogramProcessor(mFrameSource->getFrameProperties(),
                                       96, mProjectionMatrix));
  mModelOcclusionProcessor.reset(
      new ModelOcclusionProcessor(mFrameSource->getFrameProperties(),
                                  mProjectionMatrix));
  mModelDebugProcessor.reset(
      new ModelDebugProcessor(mFrameSource->getFrameProperties(),
                              mProjectionMatrix));
  mDisplaySink.reset(new DisplaySink(mGlesContext.nativeWindowDimensions().first,
                                     mGlesContext.nativeWindowDimensions().second));

  mModelDebugProcessor->setModels(models);
  const auto& debugResultSet =
      mModelDebugProcessor->process(mFrameTextureContainer.getTexture());

  mDisplaySink->send(debugResultSet);
  mGlesContext.swapBuffers();
}


void GlipfServerHandler::scanForeground(std::vector<double>& result) {
  const auto& resultSet =
      mForegroundCoverageProcessor->process(mForegroundTexture);
  const auto& foregroundCoverage =
      boost::get<vector<float>>(resultSet.at("model_coverage"));

  for (auto number : foregroundCoverage)
    result.push_back(number);
}


void GlipfServerHandler::initTarget(const glipf::Target& targetData,
                                    const bool computeRef)
{

  size_t fboWidth = mFrameSource->getFrameProperties().dimensions().first;
  size_t fboHeight = mFrameSource->getFrameProperties().dimensions().second;

  std::vector<GlesProcessor::ModelData> models = {
    generateCuboidData(targetData.pose.x, targetData.pose.y,
                       targetData.pose.z, mModelDims)
  };

  if (computeRef) {
    mForegroundHistogramProcessor->setModels(models, mProjectionMatrix);
    const auto& resultSet =
        mForegroundHistogramProcessor->process(mForegroundTexture);

    mTargetHistograms[targetData.id] =
        boost::get<vector<float>>(resultSet.at("0"));
    mTargetCoverage[targetData.id] =
        boost::get<vector<float>>(resultSet.at("histogram_coverage"))[0];
  }

  mModelDebugProcessor->setModels(models);
  mModelDebugProcessor->process(mFrameTextureContainer.getTexture());

  GLubyte pixelData[fboWidth * fboHeight * 4];
  glReadPixels(0, 0, fboWidth, fboHeight, GL_RGBA, GL_UNSIGNED_BYTE,
               pixelData);

  cv::Mat image(fboHeight, fboWidth, CV_8UC4, pixelData);
  imwrite("target-detection-debug" + std::to_string(mLastFrameNumber) + ".png",
          image);
}


bool GlipfServerHandler::isVisible(const vector<glipf::Target>& targets,
                                   const glipf::Target& newTarget)
{
  vector<GlesProcessor::ModelData> models;

  for (auto& target : targets) {
    models.push_back(generateCuboidData(target.pose.x,
                                        target.pose.y,
                                        target.pose.z, mModelDims));
  }

  models.push_back(generateCuboidData(newTarget.pose.x, newTarget.pose.y,
                                      newTarget.pose.z, mModelDims));

  mModelOcclusionProcessor->setModels(models, mProjectionMatrix);
  const auto& resultSet =
      mModelOcclusionProcessor->process(mFrameTextureContainer.getTexture());

  const auto& occlusionValues =
      boost::get<vector<float>>(resultSet.at("model_occlusion"));

  bool result = occlusionValues.back() > mVisibilityThreshold;
  mTargetOcclusionMap[newTarget.id] = !result;

  return result;
}


void GlipfServerHandler::targetUpdate(const vector<glipf::Target>& targets) {
  std::vector<GlesProcessor::ModelData> models;

  for (auto& target : targets) {
    models.push_back(generateCuboidData(target.pose.x,
                                        target.pose.y,
                                        target.pose.z, mModelDims));
  }

  mModelOcclusionProcessor->setModels(models, mProjectionMatrix);
  const auto& resultSet =
      mModelOcclusionProcessor->process(mFrameTextureContainer.getTexture());

  const auto& occlusionValues =
      boost::get<vector<float>>(resultSet.at("model_occlusion"));

  for (size_t i = 0; i < targets.size(); ++i) {
    if (occlusionValues[i] < mVisibilityThreshold) {
      mTargetOcclusionMap[targets[i].id] = true;
    } else {
      mTargetOcclusionMap[targets[i].id] = false;

      if (mTargetHistograms.count(targets[i].id))
        continue;

      vector<GlesProcessor::ModelData> targetModels = {
        generateCuboidData(targets[i].pose.x, targets[i].pose.y,
                           targets[i].pose.z, mModelDims)
      };

      mForegroundHistogramProcessor->setModels(targetModels,
                                               mProjectionMatrix);
      const auto& resultSet =
          mForegroundHistogramProcessor->process(mForegroundTexture);

      mTargetHistograms[targets[i].id] =
          boost::get<vector<float>>(resultSet.at("0"));
      mTargetCoverage[targets[i].id] =
          boost::get<vector<float>>(resultSet.at("histogram_coverage"))[0];
    }
  }
}


double GlipfServerHandler::computeBhattDist(const vector<float>& refHist,
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


void GlipfServerHandler::computeDistance(vector<double>& result,
                                         const vector<glipf::Particle>& particles)
{
  std::vector<GlesProcessor::ModelData> models;

  for (auto& particle : particles) {
    models.push_back(generateCuboidData(particle.pose.x, particle.pose.y,
                                        particle.pose.z, mModelDims));
  }

  mForegroundHistogramProcessor->setModels(models, mProjectionMatrix);
  const auto& resultSet = mForegroundHistogramProcessor->process(mForegroundTexture);
  size_t i = 0;
  const auto& histogramCoverage =
      boost::get<vector<float>>(resultSet.at("histogram_coverage"));

  for (auto& particle : particles) {
    if (!mTargetHistograms.count(particle.id) || mTargetOcclusionMap[particle.id]) {
      result.push_back(-1.0);
      ++i;
      continue;
    }

    auto histKey = std::to_string(i);
    auto& refHist = mTargetHistograms.at(particle.id);
    auto& hist = boost::get<vector<float>>(resultSet.at(histKey));
    auto bhattDist = computeBhattDist(refHist, hist);

    float coverageDiffPercentage = std::abs(
        1 - histogramCoverage[i++] / mTargetCoverage[particle.id]);
    bhattDist *= 1 - coverageDiffPercentage;

    if (bhattDist <= 0)
      result.push_back(-1.0);
    else
      result.push_back(std::sqrt(1.0 - std::sqrt(bhattDist)));
  }

  mLastParticles = particles;
}


void GlipfServerHandler::drawDebugOutput(const vector<glipf::Target>& targets,
                                         const bool drawParticles)
{
  std::vector<GlesProcessor::ModelData> models;
  std::vector<uint_fast8_t> modelGroups;

  if (drawParticles) {
    for (auto& particle : mLastParticles) {
      models.push_back(generateWireframeModel(particle.pose.x, particle.pose.y,
                                              particle.pose.z, mModelDims));
      modelGroups.push_back(1 + particle.id);
    }
  }

  for (auto& target : targets) {
    models.push_back(generateWireframeModel(target.pose.x, target.pose.y,
                                            target.pose.z, mModelDims));
    modelGroups.push_back(0);
  }

  mModelDebugProcessor->setModels(models, modelGroups);
  const auto& resultSet =
      mModelDebugProcessor->process(mFrameTextureContainer.getTexture());

  mDisplaySink->send(resultSet);
  mGlesContext.swapBuffers();
}
