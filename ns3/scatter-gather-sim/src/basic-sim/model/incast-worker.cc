#include "ns3/address.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/tcp-socket.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/tcp-socket-factory.h"
#include <iostream>

#include "incast-worker.h"

namespace ns3 {
    NS_LOG_COMPONENT_DEFINE ("IncastWorker");

    NS_OBJECT_ENSURE_REGISTERED (IncastWorker);

TypeId IncastWorker::GetTypeId (void) {
    static TypeId tid = TypeId ("ns3::IncastWorker")
            .SetParent<Application> ()
            .SetGroupName("Applications")
            .AddConstructor<IncastWorker> ()
            .AddAttribute ("Protocol",
                           "The type id of the protocol to use for the rx socket.",
                           TypeIdValue (TcpSocketFactory::GetTypeId ()),
                           MakeTypeIdAccessor (&IncastWorker::m_tid),
                           MakeTypeIdChecker ())
            .AddAttribute ("ProcessingTime",
                           "Time necessary to create a response (in microseconds).",
                           UintegerValue (0),
                           MakeUintegerAccessor (&IncastWorker::m_processing_time),
                           MakeUintegerChecker<uint32_t> ())
    ;
    return tid;
}

IncastWorker::IncastWorker ()
{
    NS_LOG_FUNCTION (this);
}

IncastWorker::~IncastWorker () {
    NS_LOG_FUNCTION(this);
}

void IncastWorker::SetFrontends(std::vector<Address> frontends){
    Worker::SetFrontends(frontends);
    left_to_send = std::vector<int>(m_frontends.size());
}

void IncastWorker::SendResponse(int index) {
    NS_LOG_FUNCTION (this);

   while (left_to_send[index])
    { // Time to send more
        int toSend = left_to_send[index];

        Ptr<Packet> packet = Create<Packet> (left_to_send[index]);
        int actual = m_sockets[index]->Send (packet);

        if (actual > 0) {
            left_to_send[index] -= actual;
        }

        if (actual != toSend)
        {
            break;
        }
    }
}

void IncastWorker::SendIfData (Ptr<Socket> socket, uint32_t bytes){
    NS_LOG_FUNCTION (this << socket);

    for (int i = 0; i < (int) left_to_send.size(); ++i) {
        if (left_to_send[i]) {
            SendResponse(i);
        }
    }
}

void IncastWorker::ReadPostProcess(Address from, Ptr<Packet> packet, uint8_t *buffer){
    int payloadSize = atoi((char*)buffer);

    // Look for the right frontend
    int frontend_index = frontend_indexes[AddressToString(from)];
    left_to_send[frontend_index] = payloadSize;

    Simulator::Schedule(MicroSeconds(m_processing_time), &IncastWorker::SendResponse, this, frontend_index);
}



}