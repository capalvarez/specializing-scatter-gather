#ifndef INCASTWORKERHELPER_H
#define INCASTWORKERHELPER_H

#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"

namespace ns3 {

/**
 * A helper to make it easier to instantiate an ns3::IncastWorker
 * on a set of nodes.
 */
class IncastWorkerHelper {
public:
    IncastWorkerHelper (Address address);

    /**
     * Return the address of the frontend to which all workers will connect
     */
    Address GetAddress();

    /**
     * Install an ns3::IncastWorker on each node of the input container
     */
    ApplicationContainer Install (NodeContainer c) const;

    /**
     * Install an ns3::IncastWorker on the node
     */
    ApplicationContainer Install (Ptr<Node> node) const;

    /**
     * Install an ns3::IncastWorker on the node
     */
    ApplicationContainer Install (std::string nodeName) const;

private:
    Ptr<Application> InstallPriv (Ptr<Node> node) const;
    ObjectFactory m_factory;
    Address m_local;
};

}

#endif
