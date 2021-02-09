#ifndef SCHEDULING_INFO_H
#define SCHEDULING_INFO_H

#include <stdint.h>

namespace ns3 {

/**
 * Summarizes one sending event in the rate algorithm implementation:
 * \param time when to send (in nanoseconds)
 * \param to_send number of bytes to send
 */
class SchedulingInfo {
public:
    double time;
    int64_t to_send;

    SchedulingInfo(){}
};

}
#endif
