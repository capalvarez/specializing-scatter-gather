#ifndef SCHEDULED_SIM_CONFIG_H
#define SCHEDULED_SIM_CONFIG_H

#include <string>
#include <stdint.h>
#include <map>
#include "simon-util.h"

namespace ns3{

    class ScheduledSimConfig {
    public:
        double safety_factor;
        double rate_tolerance;
        int64_t reference_window;
        int64_t preemptive_cancel_count;

        ScheduledSimConfig(){};

        ScheduledSimConfig(std::map<std::string, std::string> config){
            safety_factor = parse_positive_double(get_param_or_fail("safety_factor", config));
            rate_tolerance = parse_positive_double(get_param_or_fail("rate_tolerance", config));
            reference_window = parse_positive_int64(get_param_or_fail("reference_window", config));
            preemptive_cancel_count = parse_positive_int64(get_param_or_fail("preemptive_cancel_count", config));
        }
    };

}

#endif