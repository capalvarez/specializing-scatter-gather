#ifndef SCHEDULED_FRONTEND_H
#define SCHEDULED_FRONTEND_H

#include <unordered_map>

#include "ns3/frontend.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/socket.h"
#include "ns3/uinteger.h"
#include "ns3/socket-information.h"
#include "ns3/incast-timestamp.h"
#include "ns3/simulator.h"
#include "ns3/scheduled-event.h"

namespace ns3 {

class Address;
class Socket;
class Packet;
class Simulator;

/**
 * Frontend class that implements the rate algorithm
 */
class ScheduledFrontend: public Frontend {
public:
    static TypeId GetTypeId (void);

    ScheduledFrontend();
    virtual ~ScheduledFrontend();

    // Number of microseconds we wait to measure rate and issue cancellations.
    int64_t reference_window;

    // Expected RTT
    int64_t expected_rtt;

    // Proportion of the sending time that should be left as "safety" to account for variations.
    double safety_factor;
    // Tolerance for rate measurement, how much do we allow the rate to diverge from the expected one
    // before we consider it a relevant drop.
    double rate_tolerance;

    int64_t preemptive_cancel;
    int port;

    /**
     * Creates the schedule and sends the requests to the workers as fast as possible.
     */
    void SendingLoop();
protected:
    /**
     * Methods inherited from Frontend, both are empty.
     */
    void SendRequests(Ptr <Packet> packet);
    void ProcessResponse();

    /**
     * Create a packet with the information regarding the schedule
     */
    Ptr <Packet> CreatePacket(std::string schedule);

    /**
     * Reads bytes from a socket, updates the events and measures rate, issuing cancellations if necessary.
     */
    void HandleRead (Ptr<Socket> socket);
    void HandleAccept (Ptr<Socket> s, const Address& from);

    // Rate measurement: monitor rate to issue cancellations if necesary
    int64_t accum_delay;
    int accum_bytes;
    int64_t first_byte_time;
    int64_t previous_time;
    int current_round;
    int cancelling_counter;

    // List of all scheduled events (in order)
    std::vector<ScheduledEvent> scheduled_events;

    // Map associating worker and events
    std::vector<std::vector<int>> events_per_worker;

    // Last event
    int current_event;

    // Expected rate per round
    std::vector<double> rate_per_round;
};

}




#endif
