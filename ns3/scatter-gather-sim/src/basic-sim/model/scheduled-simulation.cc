#include "scheduled-simulation.h"
#include <cmath>


namespace ns3 {
    int scheduled_simulation(std::string run_dir) {
        std::map<std::string, std::string> config = ScatterGatherBase::read_and_print_config(run_dir + "/" + "config.properties");
        ScheduledSimConfig scheduledConfig(config);

        SimulatorConfig simulationConfig = ScatterGatherBase::set_configs(run_dir, config);

        ////////////////////////////////////////
        // Setup topology
        //

        int number_of_workers = simulationConfig.workers_init;

        while (number_of_workers <= simulationConfig.workers_end) {
            printf("NUMBER OF WORKERS: %d\n", number_of_workers);
            printf("SETTING UP TOPOLOGY\n");

            NodeContainer nodes = ScatterGatherBase::one_switch_scenario(simulationConfig, number_of_workers);

            ////////////////////////////////////////
            // Schedule traffic
            printf("SCHEDULING TRAFFIC\n");

            // Install Frontend App
            printf("  > Setting up frontend \n");

            ApplicationContainer frontend = ScatterGatherBase::create_scheduled_frontend(simulationConfig, scheduledConfig, number_of_workers, nodes);
            ////////////////////////////////////////
            // Perform simulation
            //
            printf("RUNNING THE SIMULATION\n");

            int64_t simulator_start_time =  ScatterGatherBase::run_simulation();

            ////////////////////////////////////////
            // Store completion times
            //

            Ptr <ScheduledFrontend> frontendApp = ((frontend.Get(0))->GetObject<ScheduledFrontend>());

            std::unordered_map <std::string, IncastTimestamp> timestamps = frontendApp->getTimestamps();
            ScatterGatherBase::print_results(timestamps, simulationConfig, number_of_workers, simulator_start_time);

            ////////////////////////////////////////
            // End simulation
            //
            Simulator::Destroy();
            printf("  > Simulator is destroyed\n");

            number_of_workers += simulationConfig.workers_step;
            printf("-------------------------------\n");
        }

        return 0;
    }


}