#ifndef IPUINTERFACE_H_
#define IPUINTERFACE_H_

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift-gen-cpp/glipf-server/GlipfServer.h>

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

namespace middleware
{

class IpuInterface
{
public:

    IpuInterface();

    ~IpuInterface();

    void init(std::vector<std::string >& ipu_ip_addresses);

    std::vector<glipf::GlipfServerClient* >& getClients();


public:

    std::vector<boost::shared_ptr<TTransport> > sockets;
    std::vector<boost::shared_ptr<TTransport> > transports;
    std::vector<glipf::GlipfServerClient* > clients;
};

} //namespace middleware


#endif /*IPUINTERFACE_H_*/
