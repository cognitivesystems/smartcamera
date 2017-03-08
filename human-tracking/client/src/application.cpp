#include "thrift-gen-cpp/GlipfServer.h"

#include <boost/program_options.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include <fstream>


using std::string;
using std::unique_ptr;
using std::vector;

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

namespace bpo = boost::program_options;


void displayScanResults(const vector<vector<double>>& resultVectors) {
  cv::Mat image(16 * 50, 16 * 50, cv::DataType<float>::type);
  vector<vector<double>::const_iterator> resultIterators;

  for (auto& resultVector : resultVectors)
    resultIterators.push_back(std::begin(resultVector));

  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j) {
      double value = 1.0;

      for (auto& resultIterator : resultIterators) {
        value *= *(resultIterator++);
      }

      if (value > 0.3)
        value = 0.7;
      else
        value = 0.0;

      cv::rectangle(image, cv::Point(50 * j, 50 * i),
                    cv::Point(50 * (j + 1), 50 * (i + 1)),
                    cv::Scalar(value),
                    CV_FILLED);
    }

  cv::imshow("Display window", image);
  cv::waitKey(1);
}


int main(int argc, char* argv[]) {
  bpo::options_description config("Configuration");
  config.add_options()
      ("server-address,s", bpo::value<vector<string>>()->composing(),
       "server address");

  bpo::positional_options_description positionalOpts;
  positionalOpts.add("server-address", -1);

  bpo::variables_map varMap;
  store(bpo::command_line_parser(argc, argv).
        options(config).positional(positionalOpts).run(), varMap);
  notify(varMap);

  std::ifstream ifs("client.cfg");

  if (ifs) {
    store(parse_config_file(ifs, config), varMap);
    notify(varMap);
  }

  vector<boost::shared_ptr<TTransport>> transports;
  vector<unique_ptr<glipf::GlipfServerClient>> clients;

  if (varMap.count("server-address")) {
    std::cout << "Server addresses are: ";

    for (auto& addr : varMap["server-address"].as<vector<string>>()) {
      std::cout << addr << " ";

      boost::shared_ptr<TTransport> socket(new TSocket(addr, 9090));
      boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
      transports.push_back(transport);
      boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
      clients.emplace_back(new glipf::GlipfServerClient(protocol));
    }

    std::cout << std::endl;
  }

  vector<glipf::Point3d> modelCenters;
  glipf::Dims modelDimensions;

  modelDimensions.x = 300.0;
  modelDimensions.y = 300.0;
  modelDimensions.z = 1750.0;

  for (int i = -8; i < 8; ++i) {
    for (int j = -8; j < 8; ++j) {
      modelCenters.emplace_back();
      glipf::Point3d& modelCenter = modelCenters.back();
      modelCenter.x = i * modelDimensions.x;
      modelCenter.y = j * modelDimensions.y;
      modelCenter.z = modelDimensions.z / 2.0;
    }
  }

  try {
    cv::namedWindow("Display window", cv::WINDOW_AUTOSIZE);

    for (auto& transport : transports)
      transport->open();

    for (auto& client : clients)
      client->initForegroundCoverageProcessor(modelCenters, modelDimensions);

    glipf::Target target;
    target.id = 42;
    target.initialPose.x = 0;
    target.initialPose.y = 0;
    target.initialPose.z = 875;

    while (true) {
      vector<vector<double>> resultVectors(clients.size());

      for (auto& client : clients)
        client->send_initTarget(target);
      for (auto& client : clients)
        client->recv_initTarget();

      for (auto& client : clients)
        client->send_scanForeground();

      auto resultVectorIter = std::begin(resultVectors);

      for (auto& client : clients)
        client->recv_scanForeground(*(resultVectorIter++));

      displayScanResults(resultVectors);
    }

    for (auto& transport : transports)
      transport->close();
  } catch (TException& tx) {
    std::cerr << "ERROR: " << tx.what() << "\n";
  }

  return 0;
}
