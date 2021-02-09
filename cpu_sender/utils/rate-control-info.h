#ifndef SENDER_RATE_CONTROL_INFO_H
#define SENDER_RATE_CONTROL_INFO_H

#include <vector>
#include <chrono>

/*
 * Describes a single event: a single workers sends a limited number of bytes.
 */
struct ScheduledEvent{
    // Time (from the first event) when this event is expected to happen.
    int delay;

    // Number of bytes that should be received.
    int bytes;

    // Index of the worker
    int worker_index;

    //Index of the "round" of the event. One round corresponds to all workers sending the maximum number of bytes allowed,
    // further rounds are used to support larger payloads.
    int round;

    // Has this event been completed?
    bool completed;

    // Is this event still valid: an event is invalid when it has been cancelled.
    bool status;

    ScheduledEvent(){
        delay = 0;
        bytes = 0;
        worker_index = 0;
        round = 0;
        completed = false;
        status = true;
    }
};

/*
 * Describes a single cancelling event
 */
struct CancellingInfo{
    // Worker that will receive the cancellation
    int worker_index;

    // Number of cancelling events prior to this one
    int cancelling_count;

    // Index of the particular event to be cancelled
    int event_index;

    CancellingInfo(){}
};


/*
 * All the information required by the rate control algorithm.
 */
class RateControlInfo{
public:
    // Index of the last event received
    int current_event;

    // Expected rate per round of the algorithm.
    std::vector<double> rate_per_round;

    // Ordered list of all pending scheduled events
    std::vector<ScheduledEvent> scheduled_events;

    // Ordered list of all events per worker
    std::vector<std::vector<int>> events_per_worker;

    // How far in time have we advanced. Used to compare with the delay defined in ScheduledEvents
    int accumulated_delay;

    // Configuration parameters used to compute the scheduling.
    int bandwidth;
    int init_cwnd;
    int mss;
    int mtu;
    int size_per_round;

    // Number of microseconds we wait to measure rate and issue cancellations
    int reference_window;

    // Tolerance for rate measurement, how much do we allow the rate to diverge from the expected one before we consider
    // it a relevant drop
    double rate_tolerance;
    // Safety factor for scheduling: time left between the worker responses.
    double safety_factor;

    // Number of events in the future we consider for the cancelling
    int cancel_distance;

    // Last time we measured rate
    std::chrono::high_resolution_clock::time_point previous_time;
    // Number of bytes received since the last time we measured rate
    int accumulated_bytes;

    // Have we recieved the first byte of a reply?
    bool received_first_byte;

    // Number of events that should be cancelled as soon as possible
    int should_cancel;

    // Number of cancelling events issued
    int cancelling_count;

    RateControlInfo(){
        current_event = 0;
        accumulated_delay = 0;

        accumulated_bytes = 0;
        received_first_byte = false;
        cancelling_count = 0;
        should_cancel = 0;

        bandwidth = 0;

        init_cwnd = 0;
        mss = 0;
        mtu = 0;
        size_per_round = 0;

        reference_window = 0;
        rate_tolerance = 0;
        safety_factor = 0;

        cancel_distance = 0;
    }
};



#endif
