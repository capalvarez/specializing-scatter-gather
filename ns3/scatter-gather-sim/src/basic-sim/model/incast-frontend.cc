#include <sstream>

#include "ns3/tcp-socket-factory.h"
#include "ns3/log.h"
#include "ns3/object-vector.h"
#include "ns3/address.h"
#include "incast-frontend.h"

namespace ns3{
    NS_LOG_COMPONENT_DEFINE ("IncastFrontend");

    NS_OBJECT_ENSURE_REGISTERED (IncastFrontend);

    TypeId IncastFrontend::GetTypeId (void) {
        static TypeId tid = TypeId ("ns3::IncastFrontend")
                .SetParent<Application> ()
                .SetGroupName("Applications")
                .AddConstructor<IncastFrontend> ()
                .AddAttribute ("Protocol",
                               "The type id of the protocol to use for the rx socket.",
                               TypeIdValue (TcpSocketFactory::GetTypeId ()),
                               MakeTypeIdAccessor (&IncastFrontend::m_tid),
                               MakeTypeIdChecker ())
                .AddAttribute ("Local",
                               "Local address.",
                               AddressValue (),
                               MakeAddressAccessor (&IncastFrontend::m_local),
                               MakeAddressChecker ())
                .AddAttribute ("PayloadSize",
                               "Number of bytes to request from each worker.",
                               UintegerValue (0),
                               MakeUintegerAccessor (&IncastFrontend::m_payloadSize),
                               MakeUintegerChecker<uint32_t> ())
                .AddAttribute ("Workers",
                               "Number of workers that will connect to the frontend.",
                               UintegerValue (0),
                               MakeUintegerAccessor (&IncastFrontend::m_n_workers),
                               MakeUintegerChecker<uint32_t> ())
                .AddAttribute ("RequestSpacing",
                               "Time between sending two requests (in microseconds).",
                               UintegerValue (1),
                               MakeUintegerAccessor (&IncastFrontend::m_inter_request_spacing),
                               MakeUintegerChecker<uint32_t> ())
        ;
        return tid;
    }

    IncastFrontend::IncastFrontend () : Frontend()
    {
        NS_LOG_FUNCTION (this);
    }

    IncastFrontend::~IncastFrontend () {
        NS_LOG_FUNCTION(this);
        m_listen = 0;
        connected_workers = 0;
    }

    void IncastFrontend::SendingLoop(){
        std::stringstream msg;
        msg << m_payloadSize;

        Ptr <Packet> packet = Create<Packet>((uint8_t *) msg.str().c_str(),
                                             msg.str().length());

        int idx = 0;

        for (SocketInformation s: m_workerSockets) {
            Simulator::Schedule(MicroSeconds(idx * m_inter_request_spacing), &IncastFrontend::SendRequest, this, s, packet);
            idx++;
        }
    }

    void IncastFrontend::ProcessResponse(){
        NS_LOG_FUNCTION (this);
    }
}