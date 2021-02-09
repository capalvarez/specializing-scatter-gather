/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "basic-sim.h"

namespace ns3 {

/**
 * Showing of simulation progress.
 */
int64_t sim_start_time_ns_since_epoch;
int64_t last_log_time_ns_since_epoch;
int64_t total_simulation_duration_ns;
int counter_estimate_remainder = 0;
double progress_interval_ns = 10000000000; // First one at 10s
void show_simulation_progress() {
    int64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    if (now - last_log_time_ns_since_epoch > progress_interval_ns) {
        printf(
                "%5.2f%% - Simulation Time = %.2fs ::: Wallclock Time = %.2fs\n",
                (Simulator::Now().GetSeconds() / (total_simulation_duration_ns / 1e9)) * 100.0,
                Simulator::Now().GetSeconds(),
                (now - sim_start_time_ns_since_epoch) / 1e9
                );
        if (counter_estimate_remainder % 5 == 0) { // Every 5 minutes we show estimate
            printf(
                    "Estimated wallclock time remaining: %.1f minutes\n",
                    ((total_simulation_duration_ns / 1e9 - Simulator::Now().GetSeconds()) / (Simulator::Now().GetSeconds() / ((now - sim_start_time_ns_since_epoch) / 1e9))) / 60.0
                    );
        }
        counter_estimate_remainder++;
        last_log_time_ns_since_epoch = now;
        progress_interval_ns = 60000000000; // After the first, each update is every 60s
    }
}

/**
 * Read and print config.
 *
 * @param filename  config.properties filename
 *
 * @return Config mapping
 */
std::map<std::string, std::string> read_and_print_config(const std::string& filename) {

    // Read the config
    std::map<std::string, std::string> config = read_config(filename);

    // Print full config
    printf("CONFIGURATION\n-----\nKEY                             VALUE\n");
    std::map<std::string, std::string>::iterator it;
    for ( it = config.begin(); it != config.end(); it++ ) {
        printf("%-30s  %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");

    return config;

}

/**
 * Read and print config.
 *
 * @param filename  config.properties filename
 * @param config    Target map to put in config
 *
 * @return 0 iff success, else non-zero
 */
Topology read_and_print_topology(const std::string& filename) {

    // Read the config
    Topology topology = Topology(filename);

    // Print full config
    printf("TOPOLOGY\n-----\nATTRIBUTE                  VALUE\n");
    printf("%-25s  %" PRIu64 "\n", "# Nodes", topology.num_nodes);
    printf("%-25s  %" PRIu64 "\n", "# Undirected edges", topology.num_undirected_edges);
    printf("%-25s  %" PRIu64 "\n", "# Switches", topology.switches.size());
    printf("%-25s  %" PRIu64 "\n", "# ... of which ToRs", topology.switches_which_are_tors.size());
    printf("%-25s  %" PRIu64 "\n", "# Servers", topology.servers.size());
    printf("\n");

    return topology;

}

/**
 * Read and print schedule.
 *
 * @param filename                  schedule.csv filename
 * @param topology                  Topology
 * @param simulation_end_time_ns    Simulation end time (ns)
 *
 * @return Schedule
 */
std::vector<schedule_entry_t> read_and_print_schedule(const std::string& filename, Topology& topology, int64_t simulation_end_time_ns) {

    // Read the schedule
    std::vector<schedule_entry_t> schedule = read_schedule(filename, topology.num_nodes, simulation_end_time_ns);

    // Check endpoint validity
    for (schedule_entry_t& entry : schedule) {
        if (!topology.is_valid_flow_endpoint(entry.from_node_id)) {
            throw std::invalid_argument(format_string("Invalid endpoint for a scheduled flow based on topology: %d", entry.from_node_id));
        }
        if (!topology.is_valid_flow_endpoint(entry.to_node_id)) {
            throw std::invalid_argument(format_string("Invalid endpoint for a schedule flow based on topology: %d", entry.to_node_id));
        }
    }

    // Print schedule
    printf("SCHEDULE\n");
    printf("  > Read schedule (total flow start events: %lu)\n", schedule.size());
    printf("\n");

    return schedule;

}

int basic_sim(std::string run_dir) {

    // Basis header
    printf("BASIS\n");

    // Run directory
    if (!dir_exists(run_dir)) {
        printf("Run directory \"%s\" does not exist.", run_dir.c_str());
        return 0;
    } else {
        printf("  > Run directory: %s\n", run_dir.c_str());
    }

    // Logs in run directory
    std::string logs_dir = run_dir + "/logs";
    if (dir_exists(logs_dir)) {
        printf("  > Emptying existing logs directory\n");
        remove_file_if_exists(logs_dir + "/finished.txt");
        remove_file_if_exists(logs_dir + "/flows.txt");
        remove_file_if_exists(logs_dir + "/flows.csv");
    } else {
        mkdir_if_not_exists(logs_dir);
    }
    printf("  > Logs directory: %s\n", logs_dir.c_str());
    printf("\n");

    // Config
    std::map<std::string, std::string> config = read_and_print_config(run_dir + "/" + "config.properties");

    // Topology
    Topology topology = read_and_print_topology(run_dir + "/" + get_param_or_fail("filename_topology", config));

    // End time
    int64_t simulation_end_time_ns = parse_positive_int64(get_param_or_fail("simulation_end_time_ns", config));

    // Schedule
    std::vector<schedule_entry_t> schedule = read_and_print_schedule(run_dir + "/" + get_param_or_fail("filename_schedule", config), topology, simulation_end_time_ns);

    // Seed
    int64_t simulation_seed = parse_positive_int64(get_param_or_fail("simulation_seed", config));

    // Link properties
    double link_data_rate_megabit_per_s = parse_positive_double(get_param_or_fail("link_data_rate_megabit_per_s", config));
    int64_t link_delay_ns = parse_positive_int64(get_param_or_fail("link_delay_ns", config));

    ////////////////////////////////////////
    // Configure NS-3
    //

    // Set primary seed
    ns3::RngSeedManager::SetSeed(simulation_seed);

    // Set end time
    Simulator::Stop(NanoSeconds(simulation_end_time_ns));
    total_simulation_duration_ns = simulation_end_time_ns;

    // ECMP routing

    // If you make some small adjustments in ipv4-global-routing.cc/h (copy over the replace....txt files into the src/internet/model versions)
    ns3::Config::SetDefault("ns3::Ipv4GlobalRouting::RoutingMode", StringValue ("ECMP_5_TUPLE_HASH")); // Options: "NO_ECMP", "ECMP_5_TUPLE_HASH", "ECMP_RANDOM"

    ////////////////////////////////////////
    // Setup topology
    //

    printf("SETTING UP TOPOLOGY\n");

    // Nodes
    printf("  > Create nodes and install Internet stack on each\n");
    InternetStackHelper internet;
    Ipv4AddressHelper address;
    address.SetBase("10.0.0.0", "255.255.255.0");
    NodeContainer nodes;
    nodes.Create(topology.num_nodes);
    internet.Install(nodes);

    // Set up links and assign IPs
    printf("  > Creating links\n");
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue(std::to_string(link_data_rate_megabit_per_s) + "Mbps"));
    p2p.SetChannelAttribute("Delay", TimeValue(NanoSeconds(link_delay_ns)));
    for (std::pair<int64_t, int64_t> link : topology.undirected_edges) {
        NetDeviceContainer container = p2p.Install(nodes.Get(link.first), nodes.Get(link.second));
        address.Assign(container);
        address.NewNetwork();
    }

    // Populate the routing tables
    printf("  > Populating routing tables\n");
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    printf("\n");

    ////////////////////////////////////////
    // Schedule traffic
    //

    // Schedule traffic
    printf("SCHEDULING TRAFFIC\n");

    // Install sink on each node
    printf("  > Setting up sinks\n");
    for (int i = 0; i < topology.num_nodes; i++) {
        PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 1024));
        ApplicationContainer app = sink.Install(nodes.Get(i));
        app.Start(Seconds(0.0));
    }

    // Install Source App
    std::vector<ApplicationContainer> apps;
    for (schedule_entry_t& entry : schedule) {
        FlowSendHelper source(
                "ns3::TcpSocketFactory",
                InetSocketAddress(
                            nodes.Get(entry.to_node_id)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(),
                            1024
                )
        );
        source.SetAttribute("MaxBytes", UintegerValue(entry.size_byte));
        ApplicationContainer app = source.Install(nodes.Get(entry.from_node_id));
        app.Start(NanoSeconds(entry.start_time_ns));
        apps.push_back(app);
    }

    printf("\n");

    ////////////////////////////////////////
    // Perform simulation
    //

    // Schedule progress printing
    sim_start_time_ns_since_epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    last_log_time_ns_since_epoch = sim_start_time_ns_since_epoch;
    double interval_s = 0.01;
    for (double i = interval_s; i < simulation_end_time_ns / 1e9; i += interval_s) {
        Simulator::Schedule(Seconds(i), &show_simulation_progress);
    }

    // Print not yet finished
    std::ofstream fileFinished(run_dir + "/logs/finished.txt");
    fileFinished << "No" << std::endl;
    fileFinished.close();

    // Run
    printf("Running the simulation for %.1f simulation seconds...\n", (simulation_end_time_ns / 1e9));
    Simulator::Run ();
    printf("Finished simulation.\n");
    int64_t sim_end_time_ns_since_epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    printf(
            "Simulation of %.1f seconds took in wallclock time %.1f seconds.\n\n",
            total_simulation_duration_ns / 1e9,
            (sim_end_time_ns_since_epoch - sim_start_time_ns_since_epoch) / 1e9
    );

    ////////////////////////////////////////
    // Store completion times
    //

    printf("POST PROCESSING\n");

    FILE* file_csv = fopen((run_dir + "/logs/flows.csv").c_str(), "w+");
    FILE* file_txt = fopen((run_dir + "/logs/flows.txt").c_str(), "w+");
    fprintf(
            file_txt, "%-12s%-10s%-10s%-16s%-18s%-18s%-16s%-16s%-13s%-16s%-12s%s\n",
            "Flow ID", "Source", "Target", "Size", "Start time (ns)",
            "End time (ns)", "Duration", "Sent", "Progress", "Avg. rate", "Finished?", "Metadata"
    );
    std::vector<ApplicationContainer>::iterator it = apps.begin();
    for (schedule_entry_t& entry : schedule) {

        // Retrieve statistics
        ApplicationContainer app = *it;
        Ptr<FlowSendApplication> flowSendApp = ((it->Get(0))->GetObject<FlowSendApplication>());
        bool is_finished = flowSendApp->IsFinished();
        bool is_err = flowSendApp->IsClosedByError();
        int64_t sent_byte = flowSendApp->GetAckedBytes();
        int64_t fct_ns;
        if (is_finished) {
            fct_ns = flowSendApp->GetCompletionTimeNs() - entry.start_time_ns;
        } else {
            fct_ns = simulation_end_time_ns - entry.start_time_ns;
        }

        // Write plain to the csv
        fprintf(
                file_csv, "%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%s,%s\n",
                entry.flow_id, entry.from_node_id, entry.to_node_id, entry.size_byte, entry.start_time_ns,
                entry.start_time_ns + fct_ns, fct_ns, sent_byte, is_err ? "ERR" : (is_finished ? "YES" : "DNF"), entry.metadata.c_str()
        );

        // Write nicely formatted to the text
        char str_size_megabit[100];
        sprintf(str_size_megabit, "%.2f Mbit", byte_to_megabit(entry.size_byte));
        char str_duration_ms[100];
        sprintf(str_duration_ms, "%.2f ms", nanosec_to_millisec(fct_ns));
        char str_sent_megabit[100];
        sprintf(str_sent_megabit, "%.2f Mbit", byte_to_megabit(sent_byte));
        char str_progress_perc[100];
        sprintf(str_progress_perc, "%.1f%%", ((double) sent_byte) / ((double) entry.size_byte) * 100.0);
        char str_avg_rate_megabit_per_s[100];
        sprintf(str_avg_rate_megabit_per_s, "%.1f Mbit/s", byte_to_megabit(sent_byte) / nanosec_to_sec(fct_ns));
        fprintf(
                file_txt, "%-12" PRId64 "%-10" PRId64 "%-10" PRId64 "%-16s%-18" PRId64 "%-18" PRId64 "%-16s%-16s%-13s%-16s%-12s%s\n",
                entry.flow_id, entry.from_node_id, entry.to_node_id, str_size_megabit, entry.start_time_ns,
                entry.start_time_ns + fct_ns, str_duration_ms, str_sent_megabit, str_progress_perc, str_avg_rate_megabit_per_s,
                is_err ? "ERR" : (is_finished ? "YES" : "DNF"), entry.metadata.c_str()
        );

        // Move on iterator
        it++;

    }
    fclose(file_csv);
    fclose(file_txt);

    printf("  > Flow log files have been written.\n");

    ////////////////////////////////////////
    // End simulation
    //
    Simulator::Destroy ();
    printf("  > Simulator is destroyed\n");

    // Print final finished
    std::ofstream fileFinishedEnd(run_dir + "/logs/finished.txt");
    fileFinishedEnd << "Yes" << std::endl;
    fileFinishedEnd.close();
    printf("  > Final finish is written\n\n");

    printf("Finished run successfully, exiting gracefully...\n");

    return 0;

}

}

