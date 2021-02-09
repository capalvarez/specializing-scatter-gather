#ifndef SCHEDULED_WORKER_H
#define SCHEDULED_WORKER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/uinteger.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/scheduling-info.h"
#include "ns3/double.h"
#include "ns3/worker.h"

namespace ns3 {

    class Address;
    class Socket;
    class Packet;
    class Simulator;

/**
 * Contains all information tracking the status of the rate algorithm for a particular worker
 */
struct FrontendInfo{
    // How much data is still left to send
    int left_to_send;

    // Schedule containing all expected events
    std::vector<SchedulingInfo> schedule;
    // Index of the current round
    int current_round;
    int expected_ending;

    // For cancelling events: have I received a cancellation packet?
    bool scheduled_cancel;
    //For cancelled events: time to schedule the cancelled event
    int new_delay;

    FrontendInfo(){
        left_to_send = 0;
        current_round = 0;
        expected_ending = 0;
        scheduled_cancel = false;
        new_delay = false;
    }
};

/**
 * Worker that sends their data according to the scheduled set by the rate algorithm.
 */
class ScheduledWorker: public Worker {
public:
    static TypeId GetTypeId (void);

    ScheduledWorker();
    virtual ~ScheduledWorker();

    /**
    * Set all associated frontends that should be served.
   */
    void SetFrontends(std::vector<Address> frontends);

    /**
     * Logs a change in the congestion window
     */
    void CwndChange (uint32_t oldCwnd, uint32_t newCwnd);
    double safety;
private:
    // List of all the frontend information structures
    std::vector<FrontendInfo> frontend_info;

    // Index of the worker
    int m_index;

    /**
     * Sends data if available.
     */
    void SendIfData (Ptr<Socket> socket, uint32_t bytes);

    /**
     * Sends the bytes associated to an event.
     */
    void SendResponse (int index);
    /**
     * When the worker receives a packet.
     * If the packet is a single integer it is considered a cancelling packet. It read the integer and saves
     * Otherwise, the packet is interpreted as a request. In this case, it reads the information contained in the request,
     * compute the scheduling considering the config parameters and schedules the first event.
     */
    virtual void ReadPostProcess(Address from, Ptr<Packet> packet, uint8_t *buffer);
};
}

#endif
