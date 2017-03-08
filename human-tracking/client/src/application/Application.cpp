#include "Application.h"

namespace application
{

void ApplicationParameters::parse(std::string& filename)
{
    QString name(filename.c_str());
    QFileInfo config(name);

    if(!config.exists())
    {
        std::cout<<"error reading " << filename << std::endl;
        throw;
    }

    QSettings iniFile(name, QSettings::IniFormat);

    iniFile.beginGroup("IPU");
    const QStringList ipus = iniFile.childKeys();
    ipu_count=ipus.size();
    ipu_ip_adresses.resize(ipus.size());
    for(int i=0; i<ipus.size();++i)
    {
        ipu_ip_adresses[i]=iniFile.value(ipus[i], "INVALID").toString().toStdString();
        std::cout << "ipu " <<  i << " ip address  " << ipu_ip_adresses[i] << std::endl;
    }
    iniFile.endGroup();

    iniFile.beginGroup("DETECTOR");
    detection_interval = iniFile.value("detection_interval", "0").toFloat();

    std::cout << "DETECTOR Parsed Data" << std::endl;
    std::cout << "detection_interval    " << detection_interval << std::endl;

    iniFile.endGroup();

    iniFile.beginGroup("PARTICLE_FILTER");
    tracking_threshold = iniFile.value("tracking_threshold", "1e-6").toFloat();
    std::cout << "TRACKER Parsed Data\ntracking_threshold    "
              << tracking_threshold << '\n';
    iniFile.endGroup();
}

Application::Application():
    file("targets.dat"),
    in(&file)
{
    std::string filename="config.ini";
    config.parse(filename);

    ipusInterface.init(config.ipu_ip_adresses);

    clients=ipusInterface.getClients();

    targetDetector.initIPU(clients);
    targetDetector.initDetector();

    targetTracker.init(clients);

    trackerImg=cv::Mat(1000,1000, CV_8UC3);
    trackerImg=cv::Scalar(0,0,0);

    frame_id = 3;
    detectionTimer.restart();
}

Application::~Application()
{

}

void Application::run()
{

    for(size_t i=0;i<clients.size();++i)
    {
        clients[i]->send_grabFrame();
    }

    for(size_t i=0;i<clients.size();++i)
    {
        clients[i]->recv_grabFrame();
    }

    frame_id++;

    if(detectionTimer.elapsed()>config.detection_interval)
    {
        std::cout << "Running detection" << std::endl;
        std::vector<target::Target > detections;
        targetDetector.detect(targets,detections);
        if(detections.size()>0)
        {
            for(size_t i=0;i<detections.size();++i)
            {
                addTarget(detections[i]);

                targetTracker.addTracker(detections[i]);
            }
        }
        detectionTimer.restart();
    }

    if(targets.size()>0)
    {

        std::vector<glipf::Target > ipu_targets;
        ipu_targets.resize(targets.size());

        for(std::size_t i=0;i<ipu_targets.size();++i)
        {
            ipu_targets[i].id=targets[i].targetId;
            ipu_targets[i].pose.x=targets[i].pose(0);
            ipu_targets[i].pose.y=targets[i].pose(1);
            ipu_targets[i].pose.z=targets[i].pose(2);
        }

        for(size_t i=0;i<clients.size();++i)
        {
            clients[i]->send_targetUpdate(ipu_targets);
        }

        for(size_t i=0;i<clients.size();++i)
        {
            clients[i]->recv_targetUpdate();
        }

        targetTracker.track();

        std::vector<int32_t> lostTargets;

        for (std::vector<target::Target>::const_iterator it = targets.begin();
             it != targets.end(); ++it)
        {
            if (targetTracker.getTargetLikelihood(it->targetId) <
                config.tracking_threshold)
            {
                lostTargets.push_back(it->targetId);
            }
        }

        for (std::vector<int32_t>::const_iterator it = lostTargets.begin();
             it != lostTargets.end(); ++it)
        {
            removeTarget(*it);
        }

        targetTracker.updateTargets(targets);

        targetTracker.drawDebugData(trackerImg);

        std::vector<glipf::Target > debug_targets;

        for(size_t i=0;i<targets.size();++i)
        {
            debug_targets.push_back(glipf::Target());
            debug_targets.back().id=targets[i].targetId;
            debug_targets.back().pose.x=targets[i].pose(0);
            debug_targets.back().pose.y=targets[i].pose(1);
            debug_targets.back().pose.z=targets[i].pose(2);
        }
        for(size_t i=0;i<clients.size();++i)
        {
            clients[i]->send_drawDebugOutput(debug_targets, true);
        }

        for(size_t i=0;i<clients.size();++i)
        {
            clients[i]->recv_drawDebugOutput();
        }

        recordTarget();

    }
    usleep(10000);
}

void Application::getDetectorDebugImage(cv::Mat& debug_img)
{
    targetDetector.getDebugImage(debug_img);
}

void Application::getTrackerDebugImage(cv::Mat &debug_img)
{
    debug_img=trackerImg;
}

void Application::addTarget(target::Target &target_data)
{
    std::cout << "Adding target with id " << target_data.targetId << std::endl;

    targets.push_back(target_data);
}

void Application::removeTarget(int32_t target_id)
{
    std::cout << "Removing target with id " << target_id << std::endl;

    int id;
    bool found=false;
    for(size_t i=0;i<targets.size();++i)
    {
        if(targets[i].targetId == static_cast<uint>(target_id))
        {
            found=true;
            id=i;
            break;
        }
    }

    if(found)
    {
        targetTracker.removeTracker(targets[id]);
        targets.erase(targets.begin()+id);
        std::cout << "Removed target with id " << target_id << std::endl;
    }
    else
    {
        std::cout << "Target with id " << target_id << "not found" << std::endl;

    }
}

void Application::recordTarget()
{
    static bool initDone=false;
    if(!initDone)
    {
        if (!file.open(QIODevice::ReadWrite | QIODevice::Text))
            throw;

        initDone=true;
    }

    if(initDone)
    {
        in << frame_id << " ";
        for(size_t i=0;i<targets.size();++i)
        {
            in << "(target " << targets[i].targetId << ": "
               << targets[i].pose(0) << "," << targets[i].pose(1) << ","
               << targets[i].pose(2) << ")  ";
        }
        in << "\n";

    }



}



}


