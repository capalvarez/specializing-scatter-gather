#include "scheduled-simulation-tcp-background.h"
#include <cmath>

namespace ns3 {
    int scheduled_simulation_tcp_background(std::string run_dir) {
        std::map<std::string, std::string> config = ScatterGatherBase::read_and_print_config(run_dir + "/" + "config.properties");
        ScheduledSimConfig scheduledConfig(config);

        // Background traffic
        double background_data_rate_mbps = parse_positive_double(get_param_or_fail("background_data_rate_mbps", config));
        int64_t background_flow_duration_ns = parse_positive_int64(get_param_or_fail("background_flow_duration_ns", config));
        
        SimulatorConfig simulationConfig = ScatterGatherBase::set_configs(run_dir, config);

        ////////////////////////////////////////
        // Setup topology
        //

        int number_of_workers = simulationConfig.workers_init;

        while (number_of_workers <= simulationConfig.workers_end) {
            printf("NUMBER OF WORKERS: %d\n", number_of_workers);
            printf("SETTING UP TOPOLOGY\n");

            NodeContainer nodes = ScatterGatherBase::one_switch_scenario(simulationConfig, number_of_workers + 1);

            ////////////////////////////////////////
            // Schedule traffic
            printf("SCHEDULING TRAFFIC\n");

            // Install Frontend App
            printf("  > Setting up frontend \n");

            ApplicationContainer frontend = ScatterGatherBase::create_scheduled_frontend(simulationConfig, scheduledConfig, number_of_workers, nodes);

            PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(nodes.Get(0)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), 4201));
            ApplicationContainer app = sink.Install(nodes.Get(0));
            app.Start(Seconds(0.0));

            OnOffHelper onoff ("ns3::TcpSocketFactory", Address ());
            onoff.SetAttribute ("Remote", AddressValue(InetSocketAddress(nodes.Get(0)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), 4201)));
            onoff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
            onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
            onoff.SetAttribute ("DataRate", DataRateValue(std::to_string(background_data_rate_mbps) + "Mbps"));
            onoff.SetAttribute ("PacketSize", UintegerValue (1380));

            ApplicationContainer clientApps = onoff.Install (nodes.Get(number_of_workers + 1));

            clientApps.Start (NanoSeconds (simulationConfig.sending_start_time_ns - (double) background_flow_duration_ns/2));
            clientApps.Stop (NanoSeconds (simulationConfig.sending_start_time_ns + (double) background_flow_duration_ns/2));

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