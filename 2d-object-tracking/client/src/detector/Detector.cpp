#include "Detector.h"

#include <QFileInfo>
#include <QSettings>
#include <QStringList>

#include <iostream>
#include <sstream>


using std::vector;
using namespace cv;


namespace detection
{

Detector::Detector(glipf::ThresholdContoursClient& client)
  : mClient(client)
{
    parseConifgFile();
    vector<glipf::Threshold> thresholds;

    for (size_t i = 0; i < TARGET_COLOUR_ENABLE.size(); ++i) {
        if (TARGET_COLOUR_ENABLE[i]) {
            glipf::Threshold threshold;
            threshold.lower.h = mHThresholds[i][0] / 360.0;
            threshold.upper.h = mHThresholds[i][1] / 360.0;
            threshold.lower.s = mSThresholds[i][0] / 100.0;
            threshold.upper.s = mSThresholds[i][1] / 100.0;
            threshold.lower.v = mVThresholds[i][0] / 100.0;
            threshold.upper.v = mVThresholds[i][1] / 100.0;

            thresholds.push_back(threshold);
        }
    }

    client.initThresholdProcessor(thresholds);
}

Detector::~Detector()
{
}

void Detector::parseConifgFile()
{
    QString filename("config.ini");
    QFileInfo config(filename);

    if(!config.exists())
    {
        std::cout<<"error reading config.ini file"<<std::endl;
        throw;
    }

    QSettings iniFile(filename, QSettings::IniFormat);

    iniFile.beginGroup("TARGETS");
    N_COLOURED_TARGETS = iniFile.value("N_COLOURED_TARGETS", 0).toInt();
    std::cout << "N_COLOURED_TARGETS " << N_COLOURED_TARGETS << std::endl;
    iniFile.endGroup();

    iniFile.beginGroup("TARGET_COLOUR_NAMES");
    TARGET_COLOUR_NAMES.clear();
    TARGET_COLOUR_NAMES.resize(N_COLOURED_TARGETS);
    for(unsigned int n = 0; n < N_COLOURED_TARGETS; ++n)
    {
        std::stringstream target_name;
        target_name.clear();
        target_name << "COLOUR_" << n;
        TARGET_COLOUR_NAMES[n] = iniFile.value(target_name.str().c_str(), " ").toString().toStdString();
        std::cout << "TARGET_COLOUR_NAMES " << n << " " << TARGET_COLOUR_NAMES[n] << std::endl;
    }
    iniFile.endGroup();

    iniFile.beginGroup("TARGET_COLOUR_ENABLE");
    TARGET_COLOUR_ENABLE.clear();
    TARGET_COLOUR_ENABLE.resize(N_COLOURED_TARGETS);
    for(unsigned int n = 0; n < N_COLOURED_TARGETS; ++n)
    {
        std::stringstream target_name;
        target_name.clear();
        target_name << "COLOUR_" << n;
        TARGET_COLOUR_ENABLE[n] = iniFile.value(target_name.str().c_str(), " ").toBool();
        std::cout << "TARGET_COLOUR_ENABLE " << n << " " << (int)TARGET_COLOUR_ENABLE[n] << std::endl;
    }
    iniFile.endGroup();


    mHThresholds.resize(N_COLOURED_TARGETS);
    iniFile.beginGroup("HSV_H_LOW_THRESHOLD");
    for(unsigned int n = 0; n < N_COLOURED_TARGETS; ++n)
    {
        std::stringstream name_h_lower_threshold;
        name_h_lower_threshold.clear();
        name_h_lower_threshold << "COLOUR_" << n << "_H_LOW" ;
        mHThresholds[n][0] = iniFile.value(name_h_lower_threshold.str().c_str(), " ").toInt();
        std::cout << name_h_lower_threshold.str() << " " << mHThresholds[n][0] << std::endl;
    }
    iniFile.endGroup();

    iniFile.beginGroup("HSV_H_HIGH_THRESHOLD");
    for(unsigned int n = 0; n < N_COLOURED_TARGETS; ++n)
    {
        std::stringstream name_h_higher_threshold;
        name_h_higher_threshold.clear();
        name_h_higher_threshold << "COLOUR_" << n << "_H_HIGH" ;
        mHThresholds[n][1] = iniFile.value(name_h_higher_threshold.str().c_str(), " ").toInt();
        std::cout << name_h_higher_threshold.str() << " " << mHThresholds[n][1] << std::endl;
    }
    iniFile.endGroup();

    mSThresholds.resize(N_COLOURED_TARGETS);
    iniFile.beginGroup("HSV_S_LOW_THRESHOLD");
    for(unsigned int n = 0; n < N_COLOURED_TARGETS; ++n)
    {
        std::stringstream name_s_lower_threshold;
        name_s_lower_threshold.clear();
        name_s_lower_threshold << "COLOUR_" << n << "_S_LOW" ;
        mSThresholds[n][0] = iniFile.value(name_s_lower_threshold.str().c_str(), " ").toInt();
        std::cout << name_s_lower_threshold.str() << " " << mSThresholds[n][0] << std::endl;
    }
    iniFile.endGroup();

    iniFile.beginGroup("HSV_S_HIGH_THRESHOLD");
    for(unsigned int n = 0; n < N_COLOURED_TARGETS; ++n)
    {
        std::stringstream name_s_higher_threshold;
        name_s_higher_threshold.clear();
        name_s_higher_threshold << "COLOUR_" << n << "_S_HIGH" ;
        mSThresholds[n][1] = iniFile.value(name_s_higher_threshold.str().c_str(), " ").toInt();
        std::cout << name_s_higher_threshold.str() << " " << mSThresholds[n][1] << std::endl;
    }
    iniFile.endGroup();

    mVThresholds.resize(N_COLOURED_TARGETS);
    iniFile.beginGroup("HSV_V_LOW_THRESHOLD");
    for(unsigned int n = 0; n < N_COLOURED_TARGETS; ++n)
    {
        std::stringstream name_v_lower_threshold;
        name_v_lower_threshold.clear();
        name_v_lower_threshold << "COLOUR_" << n << "_V_LOW" ;
        mVThresholds[n][0] = iniFile.value(name_v_lower_threshold.str().c_str(), " ").toInt();
        std::cout << name_v_lower_threshold.str() << " " << mVThresholds[n][0] << std::endl;
    }
    iniFile.endGroup();

    iniFile.beginGroup("HSV_V_HIGH_THRESHOLD");
    for(unsigned int n = 0; n < N_COLOURED_TARGETS; ++n)
    {
        std::stringstream name_v_higher_threshold;
        name_v_higher_threshold.clear();
        name_v_higher_threshold << "COLOUR_" << n << "_V_HIGH" ;
        mVThresholds[n][1] = iniFile.value(name_v_higher_threshold.str().c_str(), " ").toInt();
        std::cout << name_v_higher_threshold.str() << " " << mVThresholds[n][1] << std::endl;
    }
    iniFile.endGroup();

    iniFile.beginGroup("TARGET_COLOUR_CODE");
    TARGET_COLOUR_CODE.clear();
    TARGET_COLOUR_CODE.resize(N_COLOURED_TARGETS);
    for(unsigned int n = 0; n < N_COLOURED_TARGETS; ++n)
    {
        std::stringstream target_name;
        target_name.clear();
        target_name << "COLOUR_" << n;

        std::string code_string;
        code_string = iniFile.value(target_name.str().c_str(), " ").toString().toStdString();
        std::cout << "code_string " << code_string << std::endl;

        QString code_string_qt(code_string.c_str());
        QStringList colors = code_string_qt.split("_");
        for(int i=0;i<colors.size();++i)
        {
            std::cout << "Color code " << i << " " << colors[i].toStdString() << std::endl;
        }

        if(colors.size()!=3)
        {
            std::cout << "TARGET_COLOUR_CODE in config file should be in the format B_G_R" << std::endl;
            throw;
        }
        TARGET_COLOUR_CODE[n] = cv::Vec3b((uchar)colors[0].toInt(), (uchar)colors[1].toInt(),(uchar)colors[2].toInt());


        //std::cout << TARGET_COLOUR_CODE << n << " " << TARGET_COLOUR_CODE[n] << std::endl;
    }
    iniFile.endGroup();
}


void Detector::detectObjects(std::vector<glipf::Rect>& detectedObjects)
{
    vector< vector<glipf::Rect> > result;
    mClient.getThresholdRects(result);

    for (vector< vector<glipf::Rect> >::const_iterator it = result.begin();
         it != result.end(); ++it)
    {
        for (vector<glipf::Rect>::const_iterator jt = it->begin();
             jt != it->end(); ++jt)
        {
            detectedObjects.push_back(*jt);
        }
    }
}

}
