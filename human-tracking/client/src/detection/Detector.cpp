#include "Detector.h"

namespace detection
{

void DetectorParameters::parse(std::string& filename)
{
    QString name(filename.c_str());
    QFileInfo config(name);

    if(!config.exists())
    {
        std::cout<<"error reading " << filename << std::endl;
        throw;
    }

    QSettings iniFile(name, QSettings::IniFormat);

    iniFile.beginGroup("DETECTOR");
    max_targets = iniFile.value("max_targets", "2").toInt();

    area_x_min = iniFile.value("area_x_min", "0").toFloat();
    area_y_min = iniFile.value("area_y_min", "0").toFloat();
    area_x_max = iniFile.value("area_x_max", "0").toFloat();
    area_y_max = iniFile.value("area_y_max", "0").toFloat();

    target_dim_x = iniFile.value("target_dim_x", "0").toFloat();
    target_dim_y = iniFile.value("target_dim_y", "0").toFloat();
    target_dim_z = iniFile.value("target_dim_z", "0").toFloat();

    detection_threshold = iniFile.value("detection_threshold", "0").toFloat();
    model_span = iniFile.value("model_span", "0").toFloat();
    detection_span = iniFile.value("detection_span", "0").toFloat();
    detection_interval = iniFile.value("detection_interval", "0").toFloat();

    camera_ok_count = iniFile.value("camera_ok_count", "2").toInt();

    std::cout << "Parsed Data" << std::endl;

    std::cout << "max_targets            " << max_targets << std::endl;

    std::cout << "area_x_min            " << area_x_min << std::endl;
    std::cout << "area_y_min            " << area_y_min << std::endl;
    std::cout << "area_x_max            " << area_x_max << std::endl;
    std::cout << "area_y_max            " << area_y_max << std::endl;

    std::cout << "target_dim_x          " << target_dim_x << std::endl;
    std::cout << "target_dim_y          " << target_dim_y << std::endl;
    std::cout << "target_dim_z          " << target_dim_z << std::endl;

    std::cout << "detection_threshold   " << detection_threshold << std::endl;
    std::cout << "model_span            " << model_span << std::endl;
    std::cout << "detection_span        " << detection_span << std::endl;

    std::cout << "detection_interval    " << detection_interval << std::endl;

    std::cout << "camera_ok_count    " << camera_ok_count << std::endl;

    iniFile.endGroup();

}

Detector::Detector()
{
    std::string filename="config.ini";
    configdata.parse(filename);
    targetCount=0;

}

Detector::~Detector()
{
    debugImg.release();
    modelCenters.erase(modelCenters.begin(), modelCenters.end());
}

void Detector::initIPU(std::vector<glipf::GlipfServerClient* >& clients_ipus)
{
    nCams=clients_ipus.size();
    clients=clients_ipus;
}

void Detector::initDetector()
{
    modelCenters.erase(modelCenters.begin(), modelCenters.end());

    glipf::Dims modelDims;

    modelDims.x=configdata.target_dim_x;
    modelDims.y=configdata.target_dim_y;
    modelDims.z=configdata.target_dim_z;

    x_2=configdata.target_dim_x/2.0;
    y_2=configdata.target_dim_y/2.0;
    z_2=configdata.target_dim_z/2.0;

    x_count=0;
    y_count=0;

    for (float y = configdata.area_y_min; y < configdata.area_y_max; y+=configdata.model_span)
    {
        y_count++;
        x_count=0;
        for (float x = configdata.area_x_min; x < configdata.area_x_max; x+=configdata.model_span)
        {
            x_count++;
            modelCenters.push_back(glipf::Point3d());
            modelCenters.back().x = x+x_2;
            modelCenters.back().y = y+y_2;
            modelCenters.back().z = z_2;
        }
    }

    std::cout << "x_count  y_count " << x_count << " " << y_count << std::endl;

    for(size_t i=0;i<nCams;++i)
    {
        clients[i]->send_initForegroundCoverageProcessor(modelCenters, modelDims);
    }

    for(size_t i=0;i<nCams;++i)
    {
        clients[i]->recv_initForegroundCoverageProcessor();
    }

    if(x_count>0&&y_count>0)
    {
        debugImg =cv::Mat(y_count*20,x_count*20, CV_8UC3);
    }
    else
    {
        debugImg =cv::Mat(20,20, CV_8UC3);
    }

}

bool Detector::detect(std::vector<target::Target >& targets_old, std::vector<target::Target >& detected_targets)
{

    bool detected_target=false;

    detected_targets.erase(detected_targets.begin(),detected_targets.end());

    if(targets_old.size() >= configdata.max_targets)
    {
        return detected_target;
    }
    std::vector<target::Target > targets_new;
    targets_new.erase(targets_new.begin(),targets_new.end());

    for(size_t i=0;i<nCams;++i)
    {
        clients[i]->send_scanForeground();
    }

    std::vector<std::vector<double> > resultVectors;
    resultVectors.resize(nCams);

    for(size_t i=0;i<nCams;++i)
    {
        clients[i]->recv_scanForeground(resultVectors[i]);
    }

    size_t vectors_size;

    debugImg=cv::Scalar(0,0,0);


    if(nCams>0)
    {
        vectors_size=resultVectors[0].size();

        for(size_t n=0;n<vectors_size;++n)
        {
            std::vector<int > camOkIds;
            std::vector<int > camDoRef;

            camOkIds.erase(camOkIds.begin(), camOkIds.end());
            camDoRef.erase(camDoRef.begin(), camDoRef.end());


            for(size_t i=0;i<nCams;++i)
            {
                //residual*=resultVectors[i][n];
                //std::cout << resultVectors[i][n] << " ";

                if(resultVectors[i][n]>configdata.detection_threshold)
                {
                    camOkIds.push_back(i);
                    camDoRef.push_back(true);
                }
                else
                {
                    camDoRef.push_back(false);
                }

            }
            //std::cout << "\n";

            if(camOkIds.size()>=configdata.camera_ok_count)
            {
                std::cout << "CamOk Ids " << " ";

                for(size_t i = 0; i < camOkIds.size(); ++i)
                {
                    std::cout << camOkIds[i] << " ";
                }
                std::cout << "\n";


                for(size_t i=0;i<nCams;++i)
                {
                    std::cout << resultVectors[i][n] << " " ;
                }
                std::cout << "\n";

                int x,y;
                x = n % x_count;
                y = n / x_count;

                std::cout << "x,y " << x << " " << y << std::endl;

                cv::Rect rect;
                rect.x=x*20;
                rect.y=y*20;
                rect.width=20;
                rect.height=20;

                cv::Vec3d pose;
                pose(0)=configdata.area_x_min+ (x*configdata.model_span) + x_2;
                pose(1)=configdata.area_y_min+ (y*configdata.model_span) + y_2;
                pose(2)=configdata.target_dim_z/2.0;

                cv::Vec3d dim;
                dim(0)=configdata.target_dim_x;
                dim(1)=configdata.target_dim_y;
                dim(2)=configdata.target_dim_z;

                std::cout << "Detected Pose " << pose(0) << " " << pose(1) << " " << pose(2) << std::endl;
                target::Target target_new(nCams, targetCount, dim, pose);
                if(checkDectionWithExistingTargets(targets_old, target_new))
                {
                    targets_new.push_back(target_new);
                    targetCount++;
                    targets_new.push_back(target_new);

                    //call set ref histograms
                    glipf::Target ipu_target;
                    ipu_target.id=target_new.targetId;
                    ipu_target.pose.x=target_new.pose(0);
                    ipu_target.pose.y=target_new.pose(1);
                    ipu_target.pose.z=target_new.pose(2);

                    std::cout << "Model register at " << ipu_target.pose.x << " " << ipu_target.pose.y << " " << ipu_target.pose.z << std::endl;

                    std::vector<glipf::Target > ipu_targets;
                    ipu_targets.resize(targets_old.size());

                    for(std::size_t i=0;i<ipu_targets.size();++i)
                    {
                        ipu_targets[i].id=targets_old[i].targetId;
                        ipu_targets[i].pose.x=targets_old[i].pose(0);
                        ipu_targets[i].pose.y=targets_old[i].pose(1);
                        ipu_targets[i].pose.z=targets_old[i].pose(2);
                    }

                    for(std::size_t i=0;i<clients.size();++i)
                    {
                        if (camDoRef[i])
                            clients[i]->send_isVisible(ipu_targets, ipu_target);
                    }

                    for(std::size_t i=0;i<clients.size();++i)
                    {
                        camDoRef[i]=camDoRef[i]&&clients[i]->recv_isVisible();
                    }

                    uint8_t extra_check = 0;

                    for(std::size_t i=0;i<camDoRef.size();++i)
                    {
                        if(camDoRef[i])
                        {
                            extra_check++;
                        }
                    }

                    if(extra_check>=configdata.camera_ok_count)
                    {

                        for(size_t i=0;i<nCams;++i)
                        {
                            clients[i]->send_initTarget(ipu_target, camDoRef[i]);
                        }

                        for(size_t i=0;i<nCams;++i)
                        {
                            clients[i]->recv_initTarget();
                        }

                        cv::rectangle(debugImg, rect, CV_RGB(0,255,0), CV_FILLED);

                        detected_target=true;

                        detected_targets.push_back(target_new);
                    }
                }
                else
                {
                    std::cout << "Skipping target addition as distance span constraint not satisfied" << std::endl;
                    detected_target=false;
                }
            }
        }
    }
    else
    {
        std::cout << "resultVectors size = 0" << std::endl;
    }


    return detected_target;
}

bool Detector::checkDectionWithExistingTargets(std::vector<target::Target >& targets_old, target::Target& new_target)
{
    bool success=true;
    double dist;
    for(size_t i=0;i<targets_old.size();++i)
    {
        dist=sqrt(  pow(targets_old[i].pose(0)-new_target.pose(0),2)
                    + pow(targets_old[i].pose(1)-new_target.pose(1),2)
                    + pow(targets_old[i].pose(2)-new_target.pose(2),2));

        if(dist<configdata.detection_span)
        {
            success=false;
            break;
        }
    }

    return success;
}


void Detector::getDebugImage(cv::Mat &debug_img)
{
    debug_img=debugImg;
}

}
