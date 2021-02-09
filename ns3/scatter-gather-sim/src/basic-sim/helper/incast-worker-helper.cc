#include "ns3/inet-socket-address.h"
#include "ns3/names.h"
#include "ns3/string.h"
#include "incast-worker-helper.h"

namespace ns3 {

    IncastWorkerHelper::IncastWorkerHelper (Address address)
    {
        m_local = address;

        m_factory.SetTypeId ("ns3::IncastWorker");
        m_factory.Set ("Protocol", StringValue ("ns3::TcpSocketFactory"));
        m_factory.Set ("Frontend", AddressValue (address));
    }

    Address IncastWorkerHelper::GetAddress(){
        return m_local;
    }

    ApplicationContainer
    IncastWorkerHelper::Install (Ptr<Node> node) const
    {
        return ApplicationContainer (InstallPriv (node));
    }

    ApplicationContainer
    IncastWorkerHelper::Install (std::string nodeName) const
    {
        Ptr<Node> node = Names::Find<Node> (nodeName);
        return ApplicationContainer (InstallPriv (node));
    }

    ApplicationContainer
    IncastWorkerHelper::Install (NodeContainer c) const
    {
        ApplicationContainer apps;
        for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
        {
            apps.Add (InstallPriv (*i));
        }

        return apps;
    }

    Ptr<Application>
    IncastWorkerHelper::InstallPriv (Ptr<Node> node) const
    {
        Ptr<Application> app = m_factory.Create<Application> ();
        node->AddApplication (app);

        return app;
    }

} // namespace ns3
