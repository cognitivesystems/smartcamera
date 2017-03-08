#include "Tracker.h"

#include <stdexcept>


namespace tracking
{

Tracker::Tracker()
    : targetCount(0)
    , mParticleCount(0)
{

}

Tracker::~Tracker()
{

}

void Tracker::init(std::vector<glipf::GlipfServerClient* >& clients_ipus)
{
    nCams=clients_ipus.size();
    clients=clients_ipus;
}

void Tracker::addTracker(target::Target& new_target)
{
    std::cout << "Adding tracker for target id " << new_target.targetId << std::endl;

    //add new particle filter
    targetCount++;
    trackers.push_back(new tracking::ParticleFilter(new_target.targetId));
    trackers.back()->initialize(new_target.pose);
    std::cout << "Initialized to pose " << new_target.pose(0) << " " << new_target.pose(1) << " " << new_target.pose(2) << std::endl;
    updateParticlesPerTarget();
}

void Tracker::removeTracker(target::Target &new_target)
{
    std::cout << "Removing filter with target id " << new_target.targetId << std::endl;

    int id;
    bool found=false;
    for(size_t i=0;i<trackers.size();++i)
    {
        if(new_target.targetId==trackers[i]->getFilterId())
        {
            id=i;
            found=true;
            break;
        }
    }

    if(found)
    {
        trackers.erase(trackers.begin()+id);
        targetCount--;
        updateParticlesPerTarget();
        std::cout << "Removed filter with target id " << new_target.targetId << std::endl;
    }

}

void Tracker::track()
{
    for(std::size_t i=0;i<trackers.size();++i)
    {
        trackers[i]->predict();
    }

    //call likelihood computation
    std::vector<glipf::Particle > particles;
    cv::Vec3d particle_pose;

    //set likelihoods for all trackers
    for(std::size_t i=0;i<trackers.size();++i)
    {
        for(int n = 0; n < trackers[i]->nOfParticles(); ++n)
        {
            trackers[i]->getParticlePose(n, particle_pose);
            particles.push_back(glipf::Particle());
            particles.back().id = trackers[i]->getFilterId();
            particles.back().pose.x=particle_pose(0);
            particles.back().pose.y=particle_pose(1);
            particles.back().pose.z=particle_pose(2);
//            std::cout << "Particle " << n << " " <<
//                         particles.back().pose.x << " " <<
//                         particles.back().pose.y << " " <<
//                         particles.back().pose.z << " " << std::endl;
        }
    }

    for(std::size_t i=0;i<clients.size();++i)
    {
        clients[i]->send_computeDistance(particles);
    }

    std::vector<std::vector<double > > particles_dist;
    particles_dist.resize(clients.size());

    for(std::size_t i=0;i<clients.size();++i)
        clients[i]->recv_computeDistance(particles_dist[i]);

    for(std::size_t i=0;i<trackers.size();++i)
    {
        for(int n = 0; n < trackers[i]->nOfParticles(); ++n)
        {
            double particles_dist_fused=1.0;
            for(std::size_t t=0;t<clients.size();++t)
            {
                double dist=particles_dist[t][i*trackers[i]->nOfParticles()+n];

                if(dist!=-1)
                {
                    particles_dist_fused*=dist;
                }
            }

            trackers[i]->setParticleWeightFromDistanceMeasure(n, particles_dist_fused);
        }
    }

    //correct
    for(std::size_t i=0;i<trackers.size();++i)
    {
        trackers[i]->correct();
    }

    //cv::waitKey(0);
    //update targets
}

void Tracker::updateTargets(std::vector<target::Target >& targets_data)
{
    if(targets_data.size()==trackers.size())
    {
        for(std::size_t i=0;i<trackers.size();++i)
        {
            cv::Vec3d op_pose;
            trackers[i]->getOutputPose(op_pose);
            targets_data[i].pose=op_pose;
        }
    }
    else
    {
        std::cout << "Error: target size doesnt match with tracker size" << std::endl;
        throw;
    }

}

void Tracker::updateParticlesPerTarget()
{
    if (mParticleCount == 0 && trackers.size() > 0)
        mParticleCount = trackers.back()->nOfParticles();

    uint particles_target = mParticleCount / static_cast<float>(targetCount);

    for(std::size_t i=0;i<trackers.size();++i)
    {
        trackers[i]->resizeParticleSet(particles_target);
    }

}

void Tracker::drawDebugData(cv::Mat &debug_img)
{
    debug_img=cv::Scalar(0,0,0);

    int w_2=debug_img.size().width/2.0;
    int h_2=debug_img.size().height/2.0;

    for(std::size_t i=0;i<trackers.size();++i)
    {

        cv::Vec3d particle_pose;
        for(int n = 0; n < trackers[i]->nOfParticles(); ++n)
        {
            trackers[i]->getParticlePose(n, particle_pose);
            cv::Point particle_point;
            particle_point.x=(int)(particle_pose(0)/10.0+w_2);
            particle_point.y=(int)(particle_pose(1)/10.0+h_2);


            cv::circle(debug_img, particle_point, 3, CV_RGB(255,255,255), 1);

            //std::cout << "point " << n << " " << particle_point.x << " " << particle_point.y << std::endl;
        }

        cv::Point tracked_point;

        cv::Vec3d op_pose;
        trackers[i]->getOutputPose(op_pose);

        tracked_point.x=(int)(op_pose(0)/10.0+w_2);
        tracked_point.y=(int)(op_pose(1)/10.0+h_2);

        cv::circle(debug_img, tracked_point, 3, CV_RGB(0,255,0), CV_FILLED);

    }

}

float Tracker::getTargetLikelihood(int32_t targetId) {
  for (std::vector<ParticleFilter*>::const_iterator it = trackers.begin();
       it != trackers.end(); ++it)
  {
    if ((*it)->getFilterId() == static_cast<uint>(targetId))
      return (*it)->getLikelihood();
  }

  std::ostringstream ostr;
  ostr << targetId;

  throw std::runtime_error("Target with ID " + ostr.str() + " is not known");
}

}


