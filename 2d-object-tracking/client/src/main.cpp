/*
* Tracking
* started by Suraj Nair
*/
#include "detector/Detector.h"
#include "tracker/Tracker.h"
#include "thrift-gen-cpp/threshold-contours/ThresholdContours.h"

#include <QFileInfo>
#include <QSettings>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>


using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using std::vector;
using std::string;


void detectAndTrack(glipf::ThresholdContoursClient client,
                    detection::Detector& detector,
                    vector<tracking1::Tracker*>& trackers,
                    double reinitThreshold)
{
  for (std::vector<tracking1::Tracker*>::iterator it = trackers.begin();
       it != trackers.end(); ++it)
  {
    (*it)->trackNext1();

    if ((*it)->getLikelihood() < reinitThreshold)
      trackers.erase(it);

    break;
  }

  if (!trackers.empty())
    return;

  std::vector<glipf::Rect> detectedObjects;
  detector.detectObjects(detectedObjects);

  for (std::vector<glipf::Rect>::const_iterator it = detectedObjects.begin();
       it != detectedObjects.end(); ++it)
  {
    if (!trackers.empty())
      break;

    glipf::Target target;
    target.id = 0;
    target.pose = *it;
    client.initTarget(target);

    cv::Vec3d initialPose(it->x + (it->w / 2.0), it->y + (it->h / 2.0), 1.0);
    trackers.push_back(new tracking1::Tracker(client, initialPose,
                                              cv::Size(it->w, it->h)));
  }
}


int main() {
  QString name("config.ini");

  if (!QFileInfo(name).exists()) {
      std::cerr << "Error reading `" << name.toStdString() << "` file\n";
      return -1;
  }

  QSettings iniFile(name, QSettings::IniFormat);

  iniFile.beginGroup("IPU");
  string ipu_ip_adress = iniFile.value("IP_ADDRESS",
                                       "INVALID").toString().toStdString();
  iniFile.endGroup();

  iniFile.beginGroup("PARTICLE_FILTER");
  double reinitThreshold = iniFile.value("REINIT_THRESHOLD", "0.1").toDouble();
  std::cout << "REINIT_THRESHOLD " << reinitThreshold << '\n';
  std::cout << "IPU IP_ADDRESS " << ipu_ip_adress << '\n';
  iniFile.endGroup();

  boost::shared_ptr<TTransport> socket(new TSocket(ipu_ip_adress, 9090));
  boost::shared_ptr<TTransport> transport(new TFramedTransport(socket));
  boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  glipf::ThresholdContoursClient client(protocol);
  transport->open();

  detection::Detector detector(client);
  vector<tracking1::Tracker*> trackers;

  while(true)
    detectAndTrack(client, detector, trackers, reinitThreshold);

  return 0;
}
