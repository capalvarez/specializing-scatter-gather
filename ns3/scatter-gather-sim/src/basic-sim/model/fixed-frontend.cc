#include "fixed-frontend.h"
#include <sstream>

#include "ns3/tcp-socket-factory.h"
#include "ns3/log.h"
#include "ns3/object-vector.h"
#include "ns3/address.h"

namespace ns3{
    NS_LOG_COMPONENT_DEFINE ("FixedFrontend");

    NS_OBJECT_ENSURE_REGISTERED (FixedFrontend);

    TypeId FixedFrontend::GetTypeId (void) {
        static TypeId tid = TypeId ("ns3::FixedFrontend")
                .SetParent<Application> ()
                .SetGroupName("Applications")
                .AddConstructor<FixedFrontend> ()
                .AddAttribute ("Protocol",
                               "The type id of the protocol to use for the rx socket.",
                               TypeIdValue (TcpSocketFactory::GetTypeId ()),
                               MakeTypeIdAccessor (&FixedFrontend::m_tid),
                               MakeTypeIdChecker ())
                .AddAttribute ("Local",
                               "Local address.",
                               AddressValue (),
                               MakeAddressAccessor (&FixedFrontend::m_local),
                               MakeAddressChecker ())
                .AddAttribute ("PayloadSize",
                               "Number of bytes to request from each worker.",
                               UintegerValue (0),
                               MakeUintegerAccessor (&FixedFrontend::m_payloadSize),
                               MakeUintegerChecker<uint32_t> ())
                .AddAttribute ("Workers",
                               "Number of workers that will connect to the frontend.",
                               UintegerValue (0),
                               MakeUintegerAccessor (&FixedFrontend::m_n_workers),
                               MakeUintegerChecker<uint32_t> ())
                .AddAttribute ("RequestSpacing",
                               "Time between sending two requests (in microseconds).",
                               UintegerValue (200),
                               MakeUintegerAccessor (&FixedFrontend::m_inter_request_spacing),
                               MakeUintegerChecker<uint32_t> ())
                .AddAttribute ("ConcurrentRequests",
                               "Number of concurrent inflight requests",
                               UintegerValue (0),
                               MakeUintegerAccessor (&FixedFrontend::m_tokens),
                               MakeUintegerChecker<uint32_t> ())
        ;
        return tid;
    }

    FixedFrontend::FixedFrontend () : Frontend()
    {
        NS_LOG_FUNCTION (this);
        current_index = 0;
        used_tokens = 0;
    }

    FixedFrontend::~FixedFrontend () {
        NS_LOG_FUNCTION(this);
        m_listen = 0;
        connected_workers = 0;
    }

    void FixedFrontend::SendingLoop(){
        std::stringstream msg;
        msg << m_payloadSize;

        Ptr <Packet> packet = Create<Packet>((uint8_t *) msg.str().c_str(),
                                             msg.str().length());
        int idx = 0;

        for (;current_index < (int) m_workerSockets.size(); current_index++) {
            if (used_tokens >= m_tokens){
                break;
            }

            Simulator::Schedule(NanoSeconds(idx * m_inter_request_spacing), &FixedFrontend::SendRequest, this,
                                m_workerSockets[current_index], packet);
            used_tokens++;

            idx++;
        }
    }

    void FixedFrontend::ProcessResponse(){
        NS_LOG_FUNCTION (this);
        used_tokens--;

        if(used_tokens < m_tokens){
            SendingLoop();
        }
    }
}