#ifndef DETECTOR_H_
#define DETECTOR_H_

#include <string>
#include <target/Target.h>
#include <QMutex>
#include <QSettings>
#include <QFileInfo>
#include <middleware/IpuInterface.h>

namespace detection
{

class DetectorParameters
{
public:
    void parse(std::string& filename);

public:

    float max_targets;
    float area_x_min;
    float area_y_min;
    float area_x_max;
    float area_y_max;

    float target_dim_x;
    float target_dim_y;
    float target_dim_z;

    float model_span;
    float detection_threshold;
    float detection_span;

    float detection_interval;

    size_t camera_ok_count;
};

class Detector
{
public:

    Detector();
    ~Detector();

    void initIPU(std::vector<glipf::GlipfServerClient* >& clients_ipus);

    void initDetector();

    bool detect(std::vector<target::Target >& targets_old, std::vector<target::Target >& detected_targets);

    bool checkDectionWithExistingTargets(std::vector<target::Target >& targets_old, target::Target& new_target);

    void getDebugImage(cv::Mat& debug_img);

private:

    DetectorParameters configdata;

    size_t nCams;

    std::vector<glipf::GlipfServerClient* > clients;

    std::vector<glipf::Point3d> modelCenters;

    cv::Mat debugImg;

    int x_count;
    int y_count;

    float x_2;
    float y_2;
    float z_2;

    u_int targetCount;

};


}; //namespace detection


#endif /*DETECTOR_H_*/
