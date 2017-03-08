#include "Target.h"

namespace target
{

Target::Target(u_int sensors, uint target_id, cv::Vec3d& target_dim, cv::Vec3d &initial_pose):
    sensorCount(sensors),
    targetId(target_id),
    dim(target_dim),
    pose(initial_pose)
{
    for(std::size_t i=0 ; i<sensorCount; ++i)
    {
        camOccupancy[i] = false;
        refImageSet[i] = false;
    }
}

Target::~Target()
{
    camOccupancy.erase(camOccupancy.begin(),camOccupancy.end());
    refImageSet.erase(refImageSet.begin(),refImageSet.end());
}

Target& Target::operator=(const Target &data)
{
    sensorCount=data.sensorCount;
    targetId=data.targetId;
    dim=data.dim;
    pose=data.pose;

    for(std::size_t i=0 ; i<sensorCount; ++i)
    {
        //camOccupancy[i] = data.camOccupancy[i];
        //refImageSet[i] = data.refImageSet[i];
    }

    return *this;
}
}



