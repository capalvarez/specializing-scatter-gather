#include <map>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <chrono>
#include <stdexcept>
#include "ns3/incast-simulation.h"
#include "ns3/batching-simulation.h"
#include "ns3/scheduled-simulation.h"
#include "ns3/scheduled-simulation-poisson.h"
#include "ns3/scheduled-simulation-1-bottleneck.h"
#include "ns3/scheduled-simulation-2-bottlenecks.h"
#include "ns3/scheduled-simulation-tcp-flow.h"

using namespace ns3;

int main(int argc, char *argv[]) {

    // No buffering of printf
    setbuf(stdout, nullptr);
    
    // Retrieve run directory
    CommandLine cmd;
    std::string run_dir = "";
    std::string sim_type = "";

    cmd.Usage("Usage: ./waf --run=\"main --run_dir='<path/to/run/directory>' --sim_type='<incast/batching/scheduled/scheduled-poisson/scheduled-tcp-background/scheduled-1-bottleneck/scheduled-2-bottlenecks>'\"");
    cmd.AddValue("run_dir",  "Run directory", run_dir);
    cmd.AddValue("sim_type", "Type of simulation to run", sim_type);
    cmd.Parse(argc, argv);
    if (run_dir.compare("") == 0) {
        printf("Usage: ./waf --run=\"main --run_dir='<path/to/run/directory>'\" --sim_type='<incast/batching/scheduled/scheduled-poisson/scheduled-tcp-background/scheduled-1-bottleneck/scheduled-2-bottlenecks>'\"");
        return 0;
    }

    // Start the correct type of simulation using this run directory
    if(sim_type == "incast"){
        return incast_simulation(run_dir);
    }

    if(sim_type == "batching"){
        return batching_simulation(run_dir);
    }
  
    if(sim_type == "scheduled"){
        return scheduled_simulation(run_dir);
    }

    if(sim_type == "scheduled-poisson"){
        return scheduled_simulation_poisson(run_dir);
    }

    if(sim_type == "scheduled-tcp-background"){
        return scheduled_simulation_tcp_background(run_dir);
    }

    if(sim_type == "scheduled-1-bottleneck"){
        return scheduled_simulation_1_bottleneck(run_dir);
    }

    if(sim_type == "scheduled-2-bottlenecks"){
        return scheduled_simulation_2_bottlenecks(run_dir);
    }

    printf("Unknown type of simulation. \n");
    return 0;
}
