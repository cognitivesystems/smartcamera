#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <detection/Detector.h>
#include <tracking/Tracker.h>
#include <target/Target.h>
#include <QStringList>
#include <QTime>
#include <middleware/IpuInterface.h>
#include <QFile>
#include <QTextStream>

namespace application
{

class ApplicationParameters
{
public:
    void parse(std::string& filename);

public:
    int ipu_count;
    std::vector<std::string > ipu_ip_adresses;

    float detection_interval;
    float tracking_threshold;
};

class Application
{
public:

    Application();

    ~Application();

    void run();

    void getDetectorDebugImage(cv::Mat& debug_img);

    void getTrackerDebugImage(cv::Mat& debug_img);

    void addTarget(target::Target& target_data);

    void removeTarget(int32_t target_id);

    void recordTarget();

private:

    ApplicationParameters config;

    middleware::IpuInterface ipusInterface;

    std::vector<glipf::GlipfServerClient* > clients;

    detection::Detector targetDetector;
    QTime detectionTimer;

    tracking::Tracker targetTracker;

    std::vector<target::Target > targets;

    cv::Mat trackerImg;

    uint frame_id;

    QFile file;
    QTextStream in;

};

}; //namespace application


#endif /*APPLICATION_H_*/
