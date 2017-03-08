#include "Tracker.h"

#include <QFileInfo>
#include <QSettings>
#include <QString>


using namespace std;


namespace tracking1
{

Tracker::Tracker(glipf::ThresholdContoursClient& client, cv::Vec3d pose,
                 cv::Size size)
  : mSize(size)
  , mLikelihood(1.0)
  , mClient(client)
{
    init();

    mFilter.initialize(pose);
}

Tracker::~Tracker()
{
}

void Tracker::parseConifgFile()
{
    QString filename("config.ini");
    QFileInfo config(filename);

    if(!config.exists())
    {
        std::cout<<"error reading config.ini file"<<std::endl;
        throw;
    }

    QSettings iniFile(filename, QSettings::IniFormat);

    iniFile.beginGroup("DEBUG");
    PRINT_AVG_LIKELIHOOD = iniFile.value("PRINT_AVG_LIKELIHOOD", "false").toBool();
    PRINT_MAX_LIKELIHOOD = iniFile.value("PRINT_MAX_LIKELIHOOD", "false").toBool();
    PRINT_PARTICLE_LIKELIHOOD = iniFile.value("PRINT_PARTICLE_LIKELIHOOD", "false").toBool();
    std::cout << "DEBUG PRINT_AVG_LIKELIHOOD " << PRINT_AVG_LIKELIHOOD << std::endl;
    std::cout << "DEBUG PRINT_MAX_LIKELIHOOD " << PRINT_MAX_LIKELIHOOD << std::endl;
    std::cout << "DEBUG PRINT_PARTICLE_LIKELIHOOD " << PRINT_PARTICLE_LIKELIHOOD << std::endl;
    iniFile.endGroup();
}


void Tracker::init()
{
    parseConifgFile();
}


void Tracker::trackNext1() {
  mFilter.predict();
  cv::Vec3d poseData;
  vector<glipf::Particle> particles;

  for (int i = 0; i < mFilter.nOfParticles(); i++) {
    glipf::Particle particle;
    mFilter.getParticlePose(i, poseData);

    particle.id = 0;
    particle.pose.x = poseData[0] - (mSize.width / 2) * poseData[2];
    particle.pose.y = poseData[1] - (mSize.height / 2) * poseData[2];
    particle.pose.w = mSize.width * poseData[2];
    particle.pose.h = mSize.height * poseData[2];

    particles.push_back(particle);
  }

  mFilter.getOutputPose(poseData);
  vector<glipf::Target> targets(1);
  glipf::Target& target = targets.back();
  target.id = 0;
  target.pose.x = poseData[0] - (mSize.width / 2) * poseData[2];
  target.pose.y = poseData[1] - (mSize.height / 2) * poseData[2];
  target.pose.w = mSize.width * poseData[2];
  target.pose.h = mSize.height * poseData[2];

  vector<double> distances;
  mClient.computeDistance(distances, targets, particles);
  double lambda = mFilter.getLambda();

  for (size_t i = 0; i < distances.size(); ++i) {
    double distance = distances[i];
    cv::Vec3d particlePose;
    mFilter.getParticlePose(i, particlePose);

    if (distance < 0.0) {
      mFilter.setParticleWeight(i, 0.0f);
      continue;
    }

    distance *= (poseData[2] - particlePose[2]) / 3.0 + 1.0f;
    float likelihood = exp(-0.5 * distance * distance * lambda);

    if (PRINT_PARTICLE_LIKELIHOOD)
      std::cout << "Particle " << i << " Likelihood " << likelihood << '\n';

    mFilter.setParticleWeight(i, likelihood);
  }

  mLikelihood = mFilter.getMaxLikelihood();
  mFilter.correct();

  if (PRINT_AVG_LIKELIHOOD)
    std::cout << "Avg Likelihood " << mFilter.getAvgLikelihood() << '\n';

  if (PRINT_MAX_LIKELIHOOD)
    std::cout << "Max Likelihood " << mFilter.getMaxLikelihood() << '\n';
}


cv::Size Tracker::getSize() const {
  return mSize;
}


void Tracker::getPose(cv::Vec3d& pose) {
  mFilter.getOutputPose(pose);
}


double Tracker::getLikelihood() {
  return mLikelihood;
}

}
