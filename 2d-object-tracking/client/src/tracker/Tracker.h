#ifndef TRACKER_H_
#define TRACKER_H_

#include "ParticleFilter.h"
#include "thrift-gen-cpp/threshold-contours/ThresholdContours.h"


namespace tracking1
{
class Tracker
{
public:
    Tracker(glipf::ThresholdContoursClient& client, cv::Vec3d pose,
            cv::Size size);
    ~Tracker();

    void init();

    void parseConifgFile();

    void trackNext1();

    cv::Size getSize() const;
    void getPose(cv::Vec3d& pose);
    double getLikelihood();

public:
    cv::Size mSize;

    ParticleFilter mFilter;
    double mLikelihood;

    bool PRINT_AVG_LIKELIHOOD;
    bool PRINT_MAX_LIKELIHOOD;
    bool PRINT_PARTICLE_LIKELIHOOD;

    std::string VS_TARGET_TOPIC_NAME;
    glipf::ThresholdContoursClient& mClient;
};


}; //namespace tracking1


#endif /*TRACKERDATA_H_*/
