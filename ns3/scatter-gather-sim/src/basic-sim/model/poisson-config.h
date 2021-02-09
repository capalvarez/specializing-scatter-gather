#ifndef POISSON_CONFIG_H
#define POISSON_CONFIG_H

#include <string>
#include <stdint.h>
#include <map>
#include "ns3/scheduled-sim-config.h"
#include "simon-util.h"

namespace ns3{

    class PoissonConfig {
    public:
        int64_t poisson_rate;
        int64_t number_requests;
        int64_t incast_base_start;

        ScheduledSimConfig scheduled_config;

        PoissonConfig(std::map<std::string, std::string> config){
            ScheduledSimConfig new_config(config);
            
            scheduled_config = new_config;

            poisson_rate = parse_positive_int64(get_param_or_fail("rate", config));
            number_requests = parse_positive_int64(get_param_or_fail("number_requests", config));
            incast_base_start = parse_positive_int64(get_param_or_fail("start_time", config));
        }
    };

}

#endif