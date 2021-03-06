#include "incast-simulation.h"

namespace ns3 {
    int incast_simulation(std::string run_dir) {
        ////////////////////////////////////////
        // Configure the simulation
        //

        SimulatorConfig config = ScatterGatherBase::set_configs(run_dir);

        ////////////////////////////////////////
        // Setup topology
        //

        int number_of_workers = config.workers_init;

        while (number_of_workers <= config.workers_end) {
            printf("NUMBER OF WORKERS: %d\n", number_of_workers);
            printf("SETTING UP TOPOLOGY\n");

            NodeContainer nodes = ScatterGatherBase::one_switch_scenario(config, number_of_workers);

            ////////////////////////////////////////
            // Schedule traffic
            printf("SCHEDULING TRAFFIC\n");

            // Install Frontend App
            printf("  > Setting up frontend \n");

            ApplicationContainer frontend;
            Ptr <IncastFrontend> c = CreateObject<IncastFrontend>();

            c->SetAttribute("Local", AddressValue(InetSocketAddress(Ipv4Address::GetAny(), 1024)));
            c->SetAttribute("PayloadSize", UintegerValue(config.payload_size));
            c->SetAttribute("Workers", UintegerValue(number_of_workers));

            nodes.Get(0)->AddApplication(c);
            frontend.Add(c);

            Simulator::Schedule(NanoSeconds (config.sending_start_time_ns), &IncastFrontend::SendingLoop, c);
            std::vector<Address> frontendAddress;
            frontendAddress.emplace_back(InetSocketAddress(nodes.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), 1024));

            // Install worker app on each node from 1 to n
            printf("  > Setting up workers \n");

            for (int i = 1; i < number_of_workers + 1; i++) {
                Ptr <IncastWorker> incastWorker = CreateObject<IncastWorker>();

                incastWorker->SetFrontends(frontendAddress);
                nodes.Get(i)->AddApplication(incastWorker);

                ApplicationContainer app;

                app.Add(incastWorker);
                app.Start(NanoSeconds(config.simulation_start_time_ns + 10000 * i));
            }

            ////////////////////////////////////////
            // Perform simulation
            //

            printf("RUNNING THE SIMULATION\n");

            int64_t simulator_start_time = ScatterGatherBase::run_simulation();

            ////////////////////////////////////////
            // Store completion times
            //

            Ptr <IncastFrontend> frontendApp = ((frontend.Get(0))->GetObject<IncastFrontend>());
            std::unordered_map <std::string, IncastTimestamp> timestamps = frontendApp->getTimestamps();

            ScatterGatherBase::print_results(timestamps, config, number_of_workers, simulator_start_time);

            ////////////////////////////////////////
            // End simulation
            //
            Simulator::Destroy();
            printf("  > Simulator is destroyed\n");

            number_of_workers += config.workers_step;
            printf("-------------------------------\n");
        }

        return 0;

    }


}