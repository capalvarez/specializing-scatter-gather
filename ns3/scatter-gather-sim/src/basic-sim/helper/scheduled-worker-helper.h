#ifndef SCHEDULEDWORKERHELPER_H
#define SCHEDULEDWORKERHELPER_H

#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"

namespace ns3 {

/**
 * A helper to make it easier to instantiate an ns3::ScheduledWorker
 * on a set of nodes.
 */
class ScheduledWorkerHelper {
public:
    ScheduledWorkerHelper (Address address);

    /**
     * Helper function used to set the underlying application attributes,
     * _not_ the socket attributes.
     */
    void SetAttribute (std::string name, const AttributeValue &value);

    /**
     * Install an ns3::ScheduledWorker on each node of the input container
     */
    ApplicationContainer Install (NodeContainer c) const;

    /**
     * Install an ns3::ScheduledWorker on the node
     */
    ApplicationContainer Install (Ptr<Node> node) const;

    /**
     * Install an ns3::ScheduledWorker on the node
     */
    ApplicationContainer Install (std::string nodeName) const;

private:
    Ptr<Application> InstallPriv (Ptr<Node> node) const;
    ObjectFactory m_factory;
};

}

#endif
