#ifndef DETECTOR_H_
#define DETECTOR_H_

#include "thrift-gen-cpp/threshold-contours/ThresholdContours.h"

#include <QMutex>
#include <opencv2/opencv.hpp>
#include <vector>

#define DEG2RAD 3.14159265/180.0


namespace detection
{

class Detector
{
public:
    Detector(glipf::ThresholdContoursClient& client);
    ~Detector();

    void detectObjects(std::vector<glipf::Rect>& result);

private:
    void parseConifgFile();

private:
    //Number of colours
    unsigned int N_COLOURED_TARGETS;

    std::vector<std::string > TARGET_COLOUR_NAMES;

    std::vector<bool > TARGET_COLOUR_ENABLE;

    //Colour Thresholds vector (each entry holds upper and lower thresholds)
    std::vector<cv::Vec2i > mHThresholds;
    std::vector<cv::Vec2i > mSThresholds;
    std::vector<cv::Vec2i > mVThresholds;

    std::vector<cv::Vec3b > TARGET_COLOUR_CODE;

    int mWidth;
    int mHeight;

    glipf::ThresholdContoursClient& mClient;
};

}; //namespace tracking


#endif /*DETECTOR_H_*/
