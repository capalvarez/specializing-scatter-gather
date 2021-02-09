#ifndef SIMULATOR_CONFIG_H
#define SIMULATOR_CONFIG_H

#include <string>
#include <stdint.h>

namespace ns3{

    class SimulatorConfig {
    public:
        std::string experiment_dir;

        int64_t workers_init;
        int64_t workers_step;
        int64_t workers_end;

        int payload_size;

        double link_data_rate_gigabit_per_s;
        int64_t link_delay_us;
        int64_t buffer_size_num_pkts;

        int64_t simulation_start_time_ns;
        int64_t sending_start_time_ns;        

        SimulatorConfig (){}
    };

}

#endif