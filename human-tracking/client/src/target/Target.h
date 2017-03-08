#ifndef TARGET_H_
#define TARGET_H_

#include <opencv2/opencv.hpp>

namespace target
{

    class Target
	{
		public:

            Target(u_int sensors, uint target_id, cv::Vec3d& target_dim, cv::Vec3d& initial_pose);

            ~Target();

            Target& operator=(const Target &data);

		public:

            u_int sensorCount;
            u_int targetId;

            cv::Vec3d dim;
			cv::Vec3d pose;
			cv::Vec3d velocity;

			//occupancy flag for each camera (if occluded in a camera view then that camera is not used for tracking)
            std::map<unsigned int, bool> camOccupancy;

			//Ref image set
            std::map<unsigned int, bool> refImageSet;

            //color for debug output
			cv::Scalar color;
	};

}; //namespace target


#endif /*TARGET_H_*/
