#include "scheduled-simulation-poisson.h"
#include <cmath>

namespace ns3 {
    int scheduled_simulation_poisson(std::string run_dir) {
        std::map<std::string, std::string> config = ScatterGatherBase::read_and_print_config(run_dir + "/" + "config.properties");
        PoissonConfig poissonConfig(config);          

        SimulatorConfig simulationConfig = ScatterGatherBase::set_configs(run_dir, config);

        ////////////////////////////////////////
        // Setup topology
        //

        int number_of_workers = simulationConfig.workers_init;

        while (number_of_workers <= simulationConfig.workers_end) {
            printf("NUMBER OF WORKERS: %d\n", number_of_workers);
            printf("SETTING UP TOPOLOGY\n");

            NodeContainer nodes = ScatterGatherBase::one_switch_scenario(simulationConfig, 2*number_of_workers);

            ////////////////////////////////////////
            // Schedule traffic
            printf("SCHEDULING TRAFFIC\n");

            // Install Frontend App
            printf("  > Setting up frontend \n");

            int64_t rtt_us = 4 * simulationConfig.link_delay_us;

            ApplicationContainer frontend;
            Ipv4Address frontendAddress = Ipv4Address::GetAny();

            std::vector<Address> frontend_addresses_first_half;
            std::vector<Address> frontend_addresses_second_half;

            for (int j = 0; j < poissonConfig.number_requests; ++j) {
                Ptr <ScheduledFrontend> c = CreateObject<ScheduledFrontend>();

                c->SetAttribute("Local", AddressValue(InetSocketAddress(frontendAddress, 1024+j)));
                c->SetAttribute("PayloadSize", UintegerValue(simulationConfig.payload_size));
                c->SetAttribute("Workers", UintegerValue(number_of_workers));
                c->expected_rtt = rtt_us;
                c->reference_window = poissonConfig.scheduled_config.reference_window;
                c->safety_factor = poissonConfig.scheduled_config.safety_factor;
                c->rate_tolerance = poissonConfig.scheduled_config.rate_tolerance;
                c->preemptive_cancel = poissonConfig.scheduled_config.preemptive_cancel_count;
                
                nodes.Get(0)->AddApplication(c);
                frontend.Add(c);

                if(j < poissonConfig.number_requests/2){
                    frontend_addresses_first_half.emplace_back(InetSocketAddress(nodes.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(),
                                                                      1024+j));
                }else{
                    frontend_addresses_second_half.emplace_back(InetSocketAddress(nodes.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(),
                                                                                 1024+j));
                }
            }

            frontend.Start(Seconds(0.0));

            for (int i = 1; i < 2*number_of_workers + 1; i++) {
                Ptr <ScheduledWorker> scheduledWorker = CreateObject<ScheduledWorker>();

                if(i < number_of_workers + 1){
                    scheduledWorker->SetFrontends(frontend_addresses_first_half);
                }else{
                    scheduledWorker->SetFrontends(frontend_addresses_second_half);
                }

                scheduledWorker->safety = poissonConfig.scheduled_config.safety_factor;

                nodes.Get(i)->AddApplication(scheduledWorker);

                ApplicationContainer app;

                app.Add(scheduledWorker);
                app.Start(NanoSeconds(simulationConfig.simulation_start_time_ns + 5000 * i));
            }
           
            ScatterGatherBase::start_poisson_sending(simulationConfig, poissonConfig, frontend, number_of_workers);

            ////////////////////////////////////////
            // Perform simulation
            //

            printf("RUNNING THE SIMULATION\n");

            int64_t simulator_start_time =  ScatterGatherBase::run_simulation();

            ////////////////////////////////////////
            // Store completion times
            //

            printf("POST PROCESSING\n");

            std::ofstream timestampFile = ScatterGatherBase::get_timestamp_file(simulationConfig, number_of_workers);

            // Retrieve information from frontend
            for (int l = 0; l < poissonConfig.number_requests; ++l) {
                Ptr <ScheduledFrontend> frontendApp = ((frontend.Get(l))->GetObject<ScheduledFrontend>());
                std::unordered_map <std::string, IncastTimestamp> timestamps = frontendApp->getTimestamps();

                ScatterGatherBase::print_result(timestamps, timestampFile, simulator_start_time);
            }

            timestampFile.close();
            printf("  > Flow log files have been written.\n");

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