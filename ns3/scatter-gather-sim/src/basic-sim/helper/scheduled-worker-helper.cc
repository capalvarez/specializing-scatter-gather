#include "ns3/inet-socket-address.h"
#include "ns3/names.h"
#include "ns3/string.h"
#include "scheduled-worker-helper.h"

namespace ns3 {

    ScheduledWorkerHelper::ScheduledWorkerHelper (Address address)
    {
        m_factory.SetTypeId ("ns3::ScheduledWorker");
        m_factory.Set ("Frontend", AddressValue (address));
    }

    void
    ScheduledWorkerHelper::SetAttribute (std::string name, const AttributeValue &value)
    {
        m_factory.Set (name, value);
    }

    ApplicationContainer
    ScheduledWorkerHelper::Install (Ptr<Node> node) const
    {
        return ApplicationContainer (InstallPriv (node));
    }

    ApplicationContainer
    ScheduledWorkerHelper::Install (std::string nodeName) const
    {
        Ptr<Node> node = Names::Find<Node> (nodeName);
        return ApplicationContainer (InstallPriv (node));
    }

    ApplicationContainer
    ScheduledWorkerHelper::Install (NodeContainer c) const
    {
        ApplicationContainer apps;
        for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
        {
            apps.Add (InstallPriv (*i));
        }

        return apps;
    }

    Ptr<Application>
    ScheduledWorkerHelper::InstallPriv (Ptr<Node> node) const
    {
        Ptr<Application> app = m_factory.Create<Application> ();
        node->AddApplication (app);

        return app;
    }

}
