#include "scheduled-frontend.h"
#include <sstream>

#include "ns3/tcp-socket-factory.h"
#include "ns3/log.h"
#include "ns3/object-vector.h"
#include "ns3/address.h"

namespace ns3{
    NS_LOG_COMPONENT_DEFINE ("ScheduledFrontend");

    NS_OBJECT_ENSURE_REGISTERED (ScheduledFrontend);

    TypeId ScheduledFrontend::GetTypeId (void) {
        static TypeId tid = TypeId ("ns3::ScheduledFrontend")
                .SetParent<Application> ()
                .SetGroupName("Applications")
                .AddConstructor<ScheduledFrontend> ()
                .AddAttribute ("Protocol",
                               "The type id of the protocol to use for the rx socket.",
                               TypeIdValue (TcpSocketFactory::GetTypeId ()),
                               MakeTypeIdAccessor (&ScheduledFrontend::m_tid),
                               MakeTypeIdChecker ())
                .AddAttribute ("Local",
                               "Local address.",
                               AddressValue (),
                               MakeAddressAccessor (&ScheduledFrontend::m_local),
                               MakeAddressChecker ())
                .AddAttribute ("PayloadSize",
                               "Number of bytes to request from each worker.",
                               UintegerValue (0),
                               MakeUintegerAccessor (&ScheduledFrontend::m_payloadSize),
                               MakeUintegerChecker<uint32_t> ())
                .AddAttribute ("Workers",
                               "Number of workers that will connect to the frontend.",
                               UintegerValue (0),
                               MakeUintegerAccessor (&ScheduledFrontend::m_n_workers),
                               MakeUintegerChecker<uint32_t> ())
                .AddAttribute ("RequestSpacing",
                               "Time between sending two requests (in microseconds).",
                               UintegerValue (1),
                               MakeUintegerAccessor (&ScheduledFrontend::m_inter_request_spacing),
                               MakeUintegerChecker<uint32_t> ())

        ;
        return tid;
    }

    ScheduledFrontend::ScheduledFrontend () : Frontend()
    {
        NS_LOG_FUNCTION (this);
        connected_workers = 0;
        accum_delay = 0;
        current_event = 0;
        first_byte_time = 0;
        previous_time = 0;
        accum_bytes = 0;
        cancelling_counter = 0;
        preemptive_cancel = 0;
        port = 0;
    }

    ScheduledFrontend::~ScheduledFrontend () {
        NS_LOG_FUNCTION(this);
        m_listen = 0;
        connected_workers = 0;
    }

    void ScheduledFrontend::HandleAccept (Ptr<Socket> s, const Address& from){
        s->SetRecvCallback (MakeCallback (&ScheduledFrontend::HandleRead, this));
        SocketInformation socketInformation;
        socketInformation.socket = s;
        socketInformation.address = from;

        IncastTimestamp newTimestamp;
        newTimestamp.worker_index = m_workerSockets.size();
        timestamps[AddressToString(from)] = newTimestamp;

        m_workerSockets.push_back (socketInformation);
        connected_workers++;
    }

    Ptr <Packet> ScheduledFrontend::CreatePacket(std::string schedule){
        std::stringstream msg;
        msg << schedule;

        return Create<Packet>((uint8_t *) msg.str().c_str(), msg.str().length());
    }


    void ScheduledFrontend::SendRequests(Ptr <Packet> packet){}

    void ScheduledFrontend::SendingLoop(){
        int n = ceil((double) m_payloadSize / (10 * 1380));

        events_per_worker = std::vector<std::vector<int>>(m_n_workers, std::vector<int>());

        double accumulated_bytes = 0;

        for (int round = 0; round < n - 1; ++round) {
            int to_send_round = 10 * 1380;
            int transmission_time = ceil((double) 8 * 10 * 1500 / 10.0);

            for (int idx = 0; idx < (int) m_n_workers; idx++) {
                ScheduledEvent scheduledEvent;
                scheduledEvent.delay = accum_delay;
                scheduledEvent.bytes_to_send = to_send_round;
                scheduledEvent.worker_index = idx;
                scheduledEvent.round = round;

                accum_delay += transmission_time * (1 + safety_factor);

                events_per_worker[idx].emplace_back(scheduled_events.size());
                scheduled_events.emplace_back(scheduledEvent);
            }

            accumulated_bytes += to_send_round;

            rate_per_round.emplace_back((8 * 10 * 1380) / (transmission_time * (1 + safety_factor)));
        }

        int64_t leftover = m_payloadSize - accumulated_bytes;

        if(leftover){
            int to_send_round = leftover;
            int transmission_time = ceil((double) 8 * leftover / 1380 * 1500  / 10.0);

            for (int idx = 0; idx < (int) m_n_workers; idx++) {
                ScheduledEvent scheduledEvent;
                scheduledEvent.delay = accum_delay;
                scheduledEvent.bytes_to_send = to_send_round;
                scheduledEvent.worker_index = idx;
                scheduledEvent.round = n-1;

                accum_delay += transmission_time * (1 + safety_factor);

                events_per_worker[idx].emplace_back(scheduled_events.size());
                scheduled_events.emplace_back(scheduledEvent);
            }

            rate_per_round.emplace_back((8 * leftover / 1380 * 1500) / (transmission_time * (1 + safety_factor)));
        }

        for (int idx = 0; idx < (int) m_n_workers; idx++) {
            std::string msg = std::to_string(idx) + ";" + std::to_string(m_n_workers) + ";" + std::to_string(m_payloadSize);

            Ptr<Packet> packet = CreatePacket(msg);

            Simulator::Schedule(MicroSeconds(idx * m_inter_request_spacing), &ScheduledFrontend::SendRequest, this,
                                m_workerSockets[idx], packet);
        }
    }

    void ScheduledFrontend::ProcessResponse(){
        NS_LOG_FUNCTION (this);
    }

    void ScheduledFrontend::HandleRead(Ptr<Socket> socket){
        NS_LOG_FUNCTION (this);

        Ptr<Packet> packet;
        Address from;
        Address localAddress;

        int64_t now = Simulator::Now().GetNanoSeconds();

        if(!first_byte_time){
            first_byte_time = now;
        }

        if(!previous_time){
            previous_time = now;
        }

        while((packet = socket->RecvFrom (from))){
            if (packet->GetSize () == 0)
            { //EOF
                break;
            }

            std::string stringAddress = AddressToString(from);

            IncastTimestamp& timestampInformation = timestamps[stringAddress];

            if (timestampInformation.bytes == 0) {
                timestampInformation.firstByte = Simulator::Now().GetNanoSeconds();
            }

            timestampInformation.bytes += packet->GetSize();
            accum_bytes += packet->GetSize();

            int received_event;

            for(int i=0; i < (int) events_per_worker[timestampInformation.worker_index].size(); i++){
                if(!scheduled_events[events_per_worker[timestampInformation.worker_index][i]].completed) {
                    received_event = events_per_worker[timestampInformation.worker_index][i];
                    break;
                }
            }

            int round = floor((double) timestampInformation.bytes / 13800);

            for(int i = 0 ; i< round; i++){
                int current_event_worker = events_per_worker[timestampInformation.worker_index][i];

                if(!scheduled_events[current_event_worker].completed){
                    scheduled_events[current_event_worker].completed = true;
                }
            }

            while(received_event > current_event){
                 current_event++;
            }

            if (now - previous_time >= reference_window) {
                int64_t delta = now - previous_time;

                double rate = accum_bytes * 8.0 / delta;

                if (rate < (1 - rate_tolerance) * rate_per_round[scheduled_events[current_event].round]) {
                    int idx = current_event;

                    while (idx < (int) scheduled_events.size() &&
                           now - first_byte_time + expected_rtt / 2 * 1000 > scheduled_events[idx].delay) {
                        idx++;
                    }

                    while (idx < (int) scheduled_events.size() && !scheduled_events[idx].status) {
                        idx++;
                    }

                    for (int i = 0; i < preemptive_cancel; ++i) {
                        if (idx >= (int) scheduled_events.size()) {
                            break;
                        }

                        if (!scheduled_events[idx].status) {
                            continue;
                        }

                        if (idx < (int) scheduled_events.size()) {
                            std::stringstream msg;
                            msg << cancelling_counter;

                            Ptr <Packet> cancellation = Create<Packet>((uint8_t *) msg.str().c_str(),
                                                                       msg.str().length());

                            m_workerSockets[scheduled_events[idx].worker_index].socket->Send(cancellation);

                            scheduled_events[idx].status = false;

                            ScheduledEvent scheduledEvent;
                            scheduledEvent.delay = accum_delay;
                            scheduledEvent.bytes_to_send = scheduled_events[idx].bytes_to_send;
                            scheduledEvent.worker_index = scheduled_events[idx].worker_index;
                            scheduledEvent.round = scheduled_events[idx].round;

                            accum_delay += ceil((double) 8 * scheduled_events[idx].bytes_to_send / 1380 * 1500 / 10.0) *
                                           (1 + safety_factor);

                            cancelling_counter++;

                            scheduled_events.emplace_back(scheduledEvent);
                        }

                        idx++;
                    }

                    previous_time = now;
                    accum_bytes = 0;
                }
            }

            if (timestampInformation.bytes == m_payloadSize) {
                timestampInformation.lastByte = Simulator::Now().GetNanoSeconds();

                ProcessResponse();
            }
        }


    }
}