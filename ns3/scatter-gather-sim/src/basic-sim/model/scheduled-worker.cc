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
#include <cmath>
#include "simon-util.h"

#include "scheduled-worker.h"

namespace ns3 {
    NS_LOG_COMPONENT_DEFINE ("ScheduledWorker");

    NS_OBJECT_ENSURE_REGISTERED (ScheduledWorker);

TypeId ScheduledWorker::GetTypeId (void) {
    static TypeId tid = TypeId ("ns3::ScheduledWorker")
            .SetParent<Application> ()
            .SetGroupName("Applications")
            .AddConstructor<ScheduledWorker> ()
            .AddAttribute ("Protocol",
                           "The type id of the protocol to use for the rx socket.",
                           TypeIdValue (TcpSocketFactory::GetTypeId ()),
                           MakeTypeIdAccessor (&ScheduledWorker::m_tid),
                           MakeTypeIdChecker ())
            .AddAttribute ("ProcessingTime",
                           "Time necessary to create a response (in microseconds).",
                           UintegerValue (0),
                           MakeUintegerAccessor (&ScheduledWorker::m_processing_time),
                           MakeUintegerChecker<uint32_t> ())
    ;
    return tid;
}

void ScheduledWorker::CwndChange (uint32_t oldCwnd, uint32_t newCwnd)
{
    NS_LOG_UNCOND (m_index << "\t" << Simulator::Now ().GetMicroSeconds () << "\t" << newCwnd);
}

ScheduledWorker::ScheduledWorker ()
{
    NS_LOG_FUNCTION (this);
    safety = 0;
}

void ScheduledWorker::SetFrontends(std::vector<Address> frontends){
    Worker::SetFrontends(frontends);
    frontend_info = std::vector<FrontendInfo>(m_frontends.size());
}

ScheduledWorker::~ScheduledWorker () {
    NS_LOG_FUNCTION(this);
}

void ScheduledWorker::SendResponse(int index) {
    NS_LOG_FUNCTION (this);

    if(frontend_info[index].scheduled_cancel){
        frontend_info[index].scheduled_cancel = false;

        SchedulingInfo cancelled;
        cancelled.time = frontend_info[index].new_delay;
        cancelled.to_send = frontend_info[index].schedule[frontend_info[index].current_round].to_send;

        frontend_info[index].schedule.emplace_back(cancelled);
        frontend_info[index].current_round++;

        int64_t to_send = frontend_info[index].schedule[frontend_info[index].current_round].time -
                frontend_info[index].schedule[frontend_info[index].current_round-1].time < 0? 0 : frontend_info[index].schedule[frontend_info[index].current_round].time - frontend_info[index].schedule[frontend_info[index].current_round-1].time;

        Simulator::Schedule(NanoSeconds(to_send), &ScheduledWorker::SendResponse, this, index);
        return;
    }

    int toSend = frontend_info[index].schedule[frontend_info[index].current_round].to_send;

    if (toSend > frontend_info[index].left_to_send){
        toSend = frontend_info[index].left_to_send;
    }

    int sending_now = toSend;

    while(sending_now){
        Ptr<Packet> packet = Create<Packet> (sending_now);
        int actual = m_sockets[index]->Send (packet);

        if (actual > 0) {
            frontend_info[index].left_to_send -= actual;
            sending_now -= actual;
        }
    }

    if(frontend_info[index].left_to_send){
        frontend_info[index].current_round++;

        Simulator::Schedule(NanoSeconds(frontend_info[index].schedule[frontend_info[index].current_round].time - frontend_info[index].schedule[frontend_info[index].current_round-1].time), &ScheduledWorker::SendResponse, this, index);
    }
}

void ScheduledWorker::SendIfData (Ptr<Socket> socket, uint32_t bytes){
    NS_LOG_FUNCTION (this << socket);
}

void ScheduledWorker::ReadPostProcess(Address from, Ptr<Packet> packet, uint8_t *buffer){
    std::string request(buffer, buffer + packet->GetSize ());
    std::vector<std::string> separated_info = split_string(request, ";");

    int frontend_index = frontend_indexes[AddressToString(from)];

    int standard_transmission = ceil((double) 8 * 10 * 1500 / 10.0) * (1 + safety);

    if(separated_info.size() == 1){
        frontend_info[frontend_index].scheduled_cancel = true;
        int number_cancelled = std::atoi(separated_info[0].c_str());

        int potential_new_delay = frontend_info[frontend_index].expected_ending + standard_transmission * number_cancelled;

        if(potential_new_delay > frontend_info[frontend_index].new_delay){
            frontend_info[frontend_index].new_delay = potential_new_delay;
        }
    } else {
        std::vector<SchedulingInfo> schedule;

        int index = std::atoi(separated_info[0].c_str());
        int64_t number_workers = std::atoi(separated_info[1].c_str());
        int payloadSize = std::atoi(separated_info[2].c_str());

        int n = ceil((double) payloadSize / (10 * 1380));

        double accumulated_bytes = 0;

        for (int round = 0; round < n - 1; ++round) {
            SchedulingInfo schedulingInfo;
            schedulingInfo.time = index * standard_transmission + round * number_workers * standard_transmission;
            schedulingInfo.to_send = 10 * 1380;

            schedule.emplace_back(schedulingInfo);
            accumulated_bytes += 10 * 1380;
        }

        int expected_ending = (n-1) * standard_transmission  * number_workers;

        int64_t leftover = payloadSize - accumulated_bytes;

        if(leftover){
            int leftover_transmission = ceil((double) 8 * leftover / 1380 * 1500  / 10.0) * (1 + safety);

            SchedulingInfo schedulingInfo;
            schedulingInfo.time = (n - 1) * number_workers * standard_transmission + leftover_transmission * index;
            schedulingInfo.to_send = leftover;

            schedule.emplace_back(schedulingInfo);
            expected_ending += number_workers * leftover_transmission;
        }

        frontend_info[frontend_index].schedule = schedule;
        frontend_info[frontend_index].expected_ending = expected_ending;
        frontend_info[frontend_index].left_to_send = payloadSize;

        // Adjust for request sending time
        int64_t first_start = schedule[0].time - index * 1000;

        if(first_start < 0){
            first_start = 0;
        }

        Simulator::Schedule(NanoSeconds(first_start), &ScheduledWorker::SendResponse, this, frontend_index);
    }
}

}