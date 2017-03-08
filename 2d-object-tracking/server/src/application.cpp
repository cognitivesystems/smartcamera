#include <fstream>

#include <bcm_host.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TNonblockingServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include <glipf/sources/v4l2-camera.h>
#include <glipf/sources/opencv-video-source.h>

#include "threshold-contours-handler.h"


using glipf::sources::FrameSource;
using glipf::sources::OpenCvVideoSource;
using glipf::sources::V4L2Camera;

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;

using std::string;
using std::vector;


int main() {
  bcm_host_init();

  // Read configuration
  std::ifstream ifs("server.json");

  boost::property_tree::ptree config;
  boost::property_tree::read_json(ifs, config);
  vector<GLfloat> intrinsicsData, extrinsicsData;
  boost::optional<string> videoFileName = config.get_optional<string>("videoFile");
  std::unique_ptr<FrameSource> frameSource;

  if (videoFileName)
    frameSource.reset(new OpenCvVideoSource(*videoFileName));
  else
    frameSource.reset(new V4L2Camera(std::make_pair(640, 480)));

  for (auto& val : config.get_child("intrinsics"))
    intrinsicsData.push_back(val.second.get_value<GLfloat>());

  for (auto& val : config.get_child("extrinsics"))
    extrinsicsData.push_back(val.second.get_value<GLfloat>());

  glm::mat4x3 intrinsicsMatrix =
      glm::transpose(glm::make_mat3x4(intrinsicsData.data()));
  glm::mat4x4 extrinsicsMatrix =
      glm::transpose(glm::make_mat4x4(extrinsicsData.data()));

  glm::mat4x3 mvpMatrix = intrinsicsMatrix * extrinsicsMatrix;
  glm::mat4 expandedProjectionMatrix(glm::vec4(mvpMatrix[0], 0.0),
                                     glm::vec4(mvpMatrix[1], 0.0),
                                     glm::vec4(mvpMatrix[2], 0.0),
                                     glm::vec4(mvpMatrix[3], 1.0));

  // Configure and start Thrift RPC server
  boost::shared_ptr<ThresholdContoursHandler> handler(new ThresholdContoursHandler(std::move(frameSource),
                                                                                   expandedProjectionMatrix));
  boost::shared_ptr<TProcessor> processor(new glipf::ThresholdContoursProcessor(handler));
  boost::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

  TNonblockingServer server(processor, protocolFactory, 9090);
  server.serve();

  return 0;
}
