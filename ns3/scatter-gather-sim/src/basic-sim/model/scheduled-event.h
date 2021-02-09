#ifndef SCHEDULED_EVENT_H
#define SCHEDULED_EVENT_H

#include <stdint.h>

namespace ns3{

/**
 * Contains all information related to one event in the schedule created for the rate algorithm
 */
class ScheduledEvent{
public:
    // Index of the worker that should send
    int worker_index;

    // Time since receiving the request that the worker should wait before starting this event
    int64_t delay;

    // Number of bytes that should be send for this event
    int bytes_to_send;

    // Status of the event: was it cancelled?
    bool status;

    // Index of the round this event belongs to
    int round;

    // Was this event completed?
    bool completed;

    ScheduledEvent(){
        status = true;
        completed = false;
    }
};

}

#endif
