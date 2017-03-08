#ifndef TRACKER_H_
#define TRACKER_H_

#include "ParticleFilter.h"
#include <target/Target.h>
#include <middleware/IpuInterface.h>


namespace tracking
{

class Tracker
{
public:

    Tracker();

    ~Tracker();

    void init(std::vector<glipf::GlipfServerClient* >& clients_ipus);

    void addTracker(target::Target& new_target);

    void removeTracker(target::Target& new_target);

    void track();

    void updateTargets(std::vector<target::Target >& targets_data);

    void updateParticlesPerTarget();

    void drawDebugData(cv::Mat& debug_img);

    float getTargetLikelihood(int32_t targetId);

public:

    std::vector<glipf::GlipfServerClient* > clients;

    std::vector <tracking::ParticleFilter* > trackers;
    uint targetCount;
    uint mParticleCount;
    uint nCams;

};


}; //namespace tracking


#endif /*TRACKER_H_*/
