#include "scheduled-simulation-1-bottleneck.h"
#include <cmath>


namespace ns3 {
    int scheduled_simulation_1_bottleneck(std::string run_dir) {
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

            // Create frontend + workers: 1 bottleneck scenario
            printf("  > Create frontend and install Internet stack \n");

            NodeContainer nodes;
            nodes.Create(2*number_of_workers + 2);

            NodeContainer switches;
            switches.Create(4);

            Ipv4AddressHelper worker_subnet;
            worker_subnet.SetBase("10.1.1.0", "255.255.255.0");

            Ipv4AddressHelper worker_subnet2;
            worker_subnet2.SetBase("10.7.1.0", "255.255.255.0");

            Ipv4AddressHelper frontend_subnet;
            frontend_subnet.SetBase("10.2.1.0", "255.255.255.0");

            Ipv4AddressHelper frontend_subnet2;
            frontend_subnet2.SetBase("10.3.1.0", "255.255.255.0");

            Ipv4AddressHelper frontend_subnet3;
            frontend_subnet3.SetBase("10.4.1.0", "255.255.255.0");

            Ipv4AddressHelper frontend_subnet4;
            frontend_subnet4.SetBase("10.5.1.0", "255.255.255.0");

            Ipv4AddressHelper frontend_subnet5;
            frontend_subnet5.SetBase("10.6.1.0", "255.255.255.0");

            InternetStackHelper internet;
            internet.Install(nodes);
            internet.Install(switches);

            // Set up links and assign IPs
            printf("  > Creating links\n");

            PointToPointHelper p2p;
            p2p.SetDeviceAttribute("DataRate", DataRateValue(std::to_string(simulationConfig.link_data_rate_gigabit_per_s) + "Gbps"));
            p2p.SetChannelAttribute("Delay", TimeValue(MicroSeconds(simulationConfig.link_delay_us)));

            PointToPointHelper switch_frontend;
            switch_frontend.SetDeviceAttribute("DataRate",
                                               DataRateValue(std::to_string(simulationConfig.link_data_rate_gigabit_per_s) + "Gbps"));
            switch_frontend.SetChannelAttribute("Delay", TimeValue(MicroSeconds(simulationConfig.link_delay_us)));

            NetDeviceContainer switch_frontend_nd = switch_frontend.Install(
                    NodeContainer(switches.Get(0), nodes.Get(0)));

            NetDeviceContainer switch_frontend_nd2 = switch_frontend.Install(
                    NodeContainer(switches.Get(0), nodes.Get(1)));

            NetDeviceContainer switch_0_1 = switch_frontend.Install(
                    NodeContainer(switches.Get(0), switches.Get(1)));

            NetDeviceContainer switch_1_2 = switch_frontend.Install(
                    NodeContainer(switches.Get(1), switches.Get(2)));

            NetDeviceContainer switch_1_3 = switch_frontend.Install(
                    NodeContainer(switches.Get(1), switches.Get(3)));

            TrafficControlHelper tch;
            tch.SetRootQueueDisc ("ns3::FifoQueueDisc", "MaxSize", QueueSizeValue(QueueSize(std::to_string(simulationConfig.buffer_size_num_pkts) + "p")));
            tch.Install (switch_frontend_nd);
            tch.Install (switch_frontend_nd2);
            tch.Install (switch_0_1);
            tch.Install (switch_1_2);
            tch.Install (switch_1_3);

            for (int i = 2; i < number_of_workers + 2; i++) {
                NetDeviceContainer link = p2p.Install(NodeContainer(nodes.Get(i), switches.Get(2)));

                tch.Install(link);

                worker_subnet.Assign(link);
                worker_subnet.NewNetwork();
            }

            for (int i = number_of_workers + 2; i < 2*number_of_workers + 2; i++) {
                NetDeviceContainer link = p2p.Install(NodeContainer(nodes.Get(i), switches.Get(3)));

                tch.Install(link);

                worker_subnet2.Assign(link);
                worker_subnet2.NewNetwork();
            }

            frontend_subnet.Assign(switch_frontend_nd);
            frontend_subnet2.Assign(switch_frontend_nd2);
            frontend_subnet3.Assign(switch_0_1);
            frontend_subnet4.Assign(switch_1_2);
            frontend_subnet5.Assign(switch_1_3);

            Ipv4GlobalRoutingHelper::PopulateRoutingTables();

            ////////////////////////////////////////
            // Schedule traffic
            printf("SCHEDULING TRAFFIC\n");

            // Install Frontend App
            printf("  > Setting up frontend \n");

            int64_t rtt_us = 8 * simulationConfig.link_delay_us;

            ApplicationContainer frontend = ScatterGatherBase::create_poisson_frontends(simulationConfig, rtt_us, poissonConfig, number_of_workers, nodes);
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