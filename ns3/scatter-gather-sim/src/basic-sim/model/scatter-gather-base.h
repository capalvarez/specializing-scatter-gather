#ifndef SCATTER_GATHER_BASE_H
#define SCATTER_GATHER_BASE_H

#include <map>
#include <unordered_map>
#include <chrono>
#include <iostream>
#include <fstream>
#include <dirent.h>
#include <string>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"

#include "ns3/incast-timestamp.h"
#include "simon-util.h"
#include "ns3/simulator-config.h"
#include "ns3/poisson-config.h"
#include "ns3/scheduled-frontend.h"
#include "ns3/scheduled-worker.h"

namespace ns3 {

class ScatterGatherBase{
public:
        static std::map<std::string, std::string> read_and_print_config(const std::string& filename);
        static void copy_file(std::string from_file, std::string to_file);
        static int count_directories (std::string directory);

        static SimulatorConfig set_configs(std::string run_dir, std::map<std::string, std::string>);
        static SimulatorConfig set_configs(std::string run_dir);
        static NodeContainer one_switch_scenario(SimulatorConfig config, int number_of_workers);
        static void print_results(std::unordered_map <std::string, IncastTimestamp> timestamps, SimulatorConfig config,
                int number_of_workers, int64_t simulator_start_time);
        static int64_t run_simulation();

        static std::ofstream get_timestamp_file(SimulatorConfig config, int number_of_workers);
        static void print_result(std::unordered_map <std::string, IncastTimestamp> timestamps, std::ofstream& timestampFile, int64_t simulator_start_time);

        static ApplicationContainer create_scheduled_frontend(SimulatorConfig simulationConfig, ScheduledSimConfig scheduledConfig, int number_of_workers, NodeContainer nodes);
        static ApplicationContainer create_poisson_frontends(SimulatorConfig simulationConfig, int64_t rtt_us, PoissonConfig poissonConfig, int number_of_workers, NodeContainer nodes);
        static void start_poisson_sending(SimulatorConfig simulationConfig, PoissonConfig poissonConfig, ApplicationContainer frontend, int number_of_workers);
};
}

#endif
