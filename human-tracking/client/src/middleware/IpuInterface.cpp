#include "IpuInterface.h"


namespace middleware
{

IpuInterface::IpuInterface()
{
}

IpuInterface::~IpuInterface()
{
    for(size_t i=0;i<transports.size();++i)
    {
        transports[i]->close();
    }

    sockets.erase(sockets.begin(), sockets.end());
    transports.erase(transports.begin(), transports.end());
    clients.erase(clients.begin(), clients.end());
}

void IpuInterface::init(std::vector<std::string >& ipu_ip_addresses)
{
    sockets.erase(sockets.begin(), sockets.end());
    transports.erase(transports.begin(), transports.end());
    clients.erase(clients.begin(), clients.end());

    for(size_t i=0;i<ipu_ip_addresses.size();++i)
    {
        sockets.push_back(boost::shared_ptr<TTransport>(new TSocket(ipu_ip_addresses[i].c_str(), 9090)));
        transports.push_back(boost::shared_ptr<TTransport>(new TFramedTransport(sockets.back())));

        TBinaryProtocol* protocol = new TBinaryProtocol(transports.back());
        protocol->setStrict(true, true);

        clients.push_back(new glipf::GlipfServerClient(boost::shared_ptr<TProtocol>(protocol)));
        transports.back()->open();
    }
}

std::vector<glipf::GlipfServerClient* >& IpuInterface::getClients()
{
    return clients;
}
}



