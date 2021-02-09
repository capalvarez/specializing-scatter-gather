#include "scatter-gather-base.h"

namespace ns3 {
std::map<std::string, std::string> ScatterGatherBase::read_and_print_config(const std::string& filename) {

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

void ScatterGatherBase::copy_file(std::string from_file, std::string to_file){
    std::ifstream  src(from_file, std::ios::binary);
    std::ofstream  dst(to_file, std::ios::binary);

    dst << src.rdbuf();
}

int ScatterGatherBase::count_directories (std::string directory){
    int count = 0;

    DIR *dir_ptr = NULL;
    struct dirent *direntp;

    if( (dir_ptr = opendir(directory.c_str())) == NULL ) return 0;

    while((direntp = readdir(dir_ptr))) {
        if (strcmp(direntp->d_name, ".") == 0 || strcmp(direntp->d_name, "..") == 0)
            continue;
        if (direntp->d_type == DT_DIR) {
            ++count;
        }
    }

    closedir(dir_ptr);

    return count;
}

SimulatorConfig ScatterGatherBase::set_configs(std::string run_dir){
    std::map<std::string, std::string> config = ScatterGatherBase::read_and_print_config(run_dir + "/" + "config.properties");
    return set_configs(run_dir, config);
}

SimulatorConfig ScatterGatherBase::set_configs(std::string run_dir, std::map<std::string, std::string> config){
    SimulatorConfig simulatorConfig;

    printf("BASIS\n");

    // Run directory
    if (!dir_exists(run_dir)) {
        printf("Run directory \"%s\" does not exist.", run_dir.c_str());
        exit(EXIT_FAILURE);
    } else {
        printf("  > Run directory: %s\n", run_dir.c_str());
    }

    // Logs in run directory
    std::string logs_dir = run_dir + "/results";
    if (!dir_exists(logs_dir)) {
        mkdir_if_not_exists(logs_dir);
    }

    int n_experiments = count_directories(logs_dir);
    simulatorConfig.experiment_dir = logs_dir + "/experiment" + std::to_string(n_experiments);

    mkdir_if_not_exists(simulatorConfig.experiment_dir);

    printf("  > Logs directory: %s\n", simulatorConfig.experiment_dir.c_str());
    printf("\n");

    // Backup config file, use same name as the experiment directory
    copy_file(run_dir + "/" + "config.properties",
              logs_dir + "/experiment" + std::to_string(n_experiments) + ".properties");

    // Start time
    simulatorConfig.simulation_start_time_ns = parse_positive_int64(get_param_or_fail("simulation_start_time_ns", config));
    simulatorConfig.sending_start_time_ns = parse_positive_int64(get_param_or_fail("sending_start_time_ns", config));

    // Seed
    int64_t simulation_seed = parse_positive_int64(get_param_or_fail("simulation_seed", config));

    // Link properties
    simulatorConfig.link_data_rate_gigabit_per_s = parse_positive_double(get_param_or_fail("link_data_rate_gigabit_per_s", config));
    simulatorConfig.link_delay_us = parse_positive_int64(get_param_or_fail("link_delay_us", config));
    simulatorConfig.buffer_size_num_pkts = parse_positive_int64(get_param_or_fail("buffer_size_num_pkts", config));

    // TCP properties
    int64_t min_rto_us = parse_positive_int64(get_param_or_fail("min_rto_us", config));
    int64_t initial_rtt_estimate_us = 3 * 4 * simulatorConfig.link_delay_us;

    // Payload size
    simulatorConfig.payload_size = (int) parse_positive_int64(get_param_or_fail("payload_size", config));
    int64_t inter_request_spacing_us = parse_positive_int64(get_param_or_fail("request_spacing_us", config));
    int64_t worker_processing_us = parse_positive_int64(get_param_or_fail("worker_processing_us", config));

    // Workers
    simulatorConfig.workers_init = parse_positive_int64(get_param_or_fail("number_workers_init", config));
    simulatorConfig.workers_step = parse_positive_int64(get_param_or_fail("workers_step", config));
    simulatorConfig.workers_end = parse_positive_int64(get_param_or_fail("number_workers_end", config));

    ////////////////////////////////////////
    // Configure NS-3
    //

    // Set primary seed
    ns3::RngSeedManager::SetSeed(simulation_seed);

    // ECMP routing
    // If you make some small adjustments in ipv4-global-routing.cc/h (copy over the replace....txt files into the src/internet/model versions)
    ns3::Config::SetDefault("ns3::Ipv4GlobalRouting::RoutingMode", StringValue ("NO_ECMP")); // Options: "NO_ECMP", "ECMP_5_TUPLE_HASH", "ECMP_RANDOM"

    ////////////////////////////////////////
    // Configuration changes to make everything worker closer to Linux: adapted from Simon's version of the project
    uint32_t init_cwnd_pkts = 10;
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(init_cwnd_pkts));

    Config::SetDefault("ns3::TcpSocketBase::ClockGranularity", TimeValue(NanoSeconds(1)));
    Config::SetDefault("ns3::RttEstimator::InitialEstimation", TimeValue(MicroSeconds(initial_rtt_estimate_us)));
    Config::SetDefault("ns3::TcpSocketBase::MinRto", TimeValue(MicroSeconds(min_rto_us)));
    Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (0));

    int64_t snd_buf_size_byte = 131072 * 256;  // 131072 bytes = 128 KiB is default, we set to 32 MiB
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(snd_buf_size_byte));

    // Receive buffer size
    int64_t rcv_buf_size_byte = 131072 * 256;  // 131072 bytes = 128 KiB is default, we set to 32 MiB
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(rcv_buf_size_byte));

    int64_t segment_size_byte = 1380;
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(segment_size_byte));

    Config::SetDefault("ns3::TcpSocketBase::Sack", BooleanValue (true));

    // Important to have proper rtt estimation
    bool opt_timestamp_enabled = false;  // Default: true.
    Config::SetDefault("ns3::TcpSocketBase::Timestamp", BooleanValue(opt_timestamp_enabled));

    // Set TCP Cubic as default
    std::string transport_prot = "ns3::TcpCubic";
    std::string recovery_prot = "ns3::TcpClassicRecovery";

    TypeId recTid;
    NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (recovery_prot, &recTid), "TypeId " << recovery_prot << " not found");
    Config::SetDefault ("ns3::TcpL4Protocol::RecoveryType", TypeIdValue (TypeId::LookupByName (recovery_prot)));

    TypeId tcpTid;
    NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (transport_prot, &tcpTid), "TypeId " << transport_prot << " not found");
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (transport_prot)));

    Config::SetDefault ("ns3::TcpCubic::Beta", DoubleValue (0.7));
    Config::SetDefault ("ns3::IncastFrontend::RequestSpacing", UintegerValue(inter_request_spacing_us));
    Config::SetDefault ("ns3::IncastWorker::ProcessingTime", UintegerValue(worker_processing_us));

    return simulatorConfig;
}

NodeContainer ScatterGatherBase::one_switch_scenario(SimulatorConfig config, int number_of_workers){
    // Create frontend + workers
    printf("  > Create frontend and install Internet stack \n");

    NodeContainer nodes;
    nodes.Create(number_of_workers + 1);

    NodeContainer switches;
    switches.Create(1);

    Ipv4AddressHelper worker_subnet;
    worker_subnet.SetBase("10.1.0.0", "255.255.252.0");

    Ipv4AddressHelper frontend_subnet;
    frontend_subnet.SetBase("10.2.1.0", "255.255.255.0");

    InternetStackHelper internet;
    internet.Install(nodes);
    internet.Install(switches);

    // Set up links and assign IPs
    printf("  > Creating links\n");

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", DataRateValue(std::to_string(config.link_data_rate_gigabit_per_s) + "Gbps"));
    p2p.SetChannelAttribute("Delay", TimeValue(MicroSeconds(config.link_delay_us)));

    PointToPointHelper switch_frontend;
    switch_frontend.SetDeviceAttribute("DataRate",
                                       DataRateValue(std::to_string(config.link_data_rate_gigabit_per_s) + "Gbps"));
    switch_frontend.SetChannelAttribute("Delay", TimeValue(MicroSeconds(config.link_delay_us)));

    NetDeviceContainer switch_frontend_nd = switch_frontend.Install(
            NodeContainer(switches.Get(0), nodes.Get(0)));

    TrafficControlHelper tch;
    tch.SetRootQueueDisc("ns3::FifoQueueDisc", "MaxSize",
                         QueueSizeValue(QueueSize(std::to_string(config.buffer_size_num_pkts) + "p")));
    QueueDiscContainer qdiscs = tch.Install(switch_frontend_nd);

    // Connect all workers to the switch
    for (int i = 1; i < number_of_workers + 1; i++) {
        NetDeviceContainer link = p2p.Install(NodeContainer(nodes.Get(i), switches.Get(0)));

        tch.Install(link);

        worker_subnet.Assign(link);
        worker_subnet.NewNetwork();
    }

    frontend_subnet.Assign(switch_frontend_nd);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    return nodes;
}

std::ofstream ScatterGatherBase::get_timestamp_file(SimulatorConfig config, int number_of_workers){
        // Use a descriptive file name
    std::ofstream timestampFile;
    std::string fileName =
            "workers" + std::to_string(number_of_workers) + "_size" + std::to_string(config.payload_size) + ".txt";

    timestampFile.open(config.experiment_dir + "/" + fileName);
    return timestampFile;
}

void ScatterGatherBase::print_result(std::unordered_map <std::string, IncastTimestamp> timestamps, std::ofstream& timestampFile, int64_t simulator_start_time){
    int64_t simulator_end_time = Simulator::Now().GetNanoSeconds();

    int64_t minSending = (simulator_end_time - simulator_start_time);
    int64_t maxReceived = 0;

    // Write results to a file
    for (auto it = timestamps.begin(); it != timestamps.end(); it++) {
        IncastTimestamp timestamp = it->second;

        int64_t sending = (timestamp.sentRequest - simulator_start_time);
        int64_t firstByte = (timestamp.firstByte - simulator_start_time);
        int64_t lastByte = (timestamp.lastByte - simulator_start_time);

        if (sending < minSending) {
            minSending = sending;
        }

        if (lastByte > maxReceived) {
            maxReceived = lastByte;
        }

        timestampFile << timestamp.worker_index << "\t" << sending << " " << firstByte << " " << lastByte
                      << std::endl;
    }

    timestampFile << "Starting time: " << minSending << std::endl;
    timestampFile << "Finishing time: " << maxReceived << std::endl;
    timestampFile << "FCT: " << maxReceived - minSending << std::endl;
    timestampFile << std::endl;
}

void ScatterGatherBase::print_results(std::unordered_map <std::string, IncastTimestamp> timestamps, SimulatorConfig config,
        int number_of_workers, int64_t simulator_start_time){
    printf("POST PROCESSING\n");

    std::ofstream timestampFile = get_timestamp_file(config, number_of_workers);

    // Retrieve information from frontend
    print_result(timestamps, timestampFile, simulator_start_time);

    timestampFile.close();

    printf("  > Flow log files have been written.\n");
}

int64_t ScatterGatherBase::run_simulation(){
    int64_t sim_start_time_ns_since_epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

    int64_t simulator_start_time = Simulator::Now().GetNanoSeconds();

    // Run
    printf("    > Starting the simulation...\n");
    Simulator::Run();
    printf("    > Finished simulation.\n");
    int64_t sim_end_time_ns_since_epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    
    printf(
            "   > Simulation took wallclock time %.1f seconds.\n",
            (sim_end_time_ns_since_epoch - sim_start_time_ns_since_epoch) / 1e9
    );

    return simulator_start_time;
}

ApplicationContainer ScatterGatherBase::create_poisson_frontends(SimulatorConfig simulationConfig, int64_t rtt_us, PoissonConfig poissonConfig, int number_of_workers, NodeContainer nodes){
    ApplicationContainer frontend;
    Ipv4Address frontendAddress0 = Ipv4Address::GetAny();
    Ipv4Address frontendAddress1 = Ipv4Address::GetAny();

    std::vector<Address> frontend_addresses_node0;
    std::vector<Address> frontend_addresses_node1;

    Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();

    for (int j = 0; j < poissonConfig.number_requests; ++j) {
        Ptr <ScheduledFrontend> c = CreateObject<ScheduledFrontend>();

        c->SetAttribute("PayloadSize", UintegerValue(simulationConfig.payload_size));
        c->SetAttribute("Workers", UintegerValue(number_of_workers));
        
        c->expected_rtt = rtt_us;
        c->reference_window = poissonConfig.scheduled_config.reference_window;
        c->safety_factor = poissonConfig.scheduled_config.safety_factor;
        c->rate_tolerance = poissonConfig.scheduled_config.rate_tolerance;
        c->preemptive_cancel = poissonConfig.scheduled_config.preemptive_cancel_count;

        // Choose at random which of the two frontends this worker should serve
        int node_index = uv->GetValue() < 0.5? 0 : 1;

        if(node_index == 0){
            nodes.Get(0)->AddApplication(c);
            c->SetAttribute("Local", AddressValue(InetSocketAddress(frontendAddress0, 1024+j)));
            frontend_addresses_node0.emplace_back(InetSocketAddress(nodes.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(),
                                                                    1024+j));
        }else{
            nodes.Get(1)->AddApplication(c);
            c->SetAttribute("Local", AddressValue(InetSocketAddress(frontendAddress1, 1024+j)));
            frontend_addresses_node1.emplace_back(InetSocketAddress(nodes.Get(1)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(),
                                                                    1024+j));
        }

        frontend.Add(c);
    }

    frontend.Start(Seconds(0.0));

    // Install worker app on each node from 1 to n
    printf("  > Setting up workers \n");

    for (int i = 2; i < 2*number_of_workers + 2; i++) {
        Ptr <ScheduledWorker> scheduledWorker = CreateObject<ScheduledWorker>();

        scheduledWorker->safety = poissonConfig.scheduled_config.safety_factor;

        if(i < number_of_workers + 2){
            scheduledWorker->SetFrontends(frontend_addresses_node0);
        }else{
            scheduledWorker->SetFrontends(frontend_addresses_node1);
        }
        
        nodes.Get(i)->AddApplication(scheduledWorker);

        ApplicationContainer app;

        app.Add(scheduledWorker);
        app.Start(NanoSeconds(simulationConfig.simulation_start_time_ns + i * 5000));
    }

    return frontend;
}

ApplicationContainer ScatterGatherBase::create_scheduled_frontend(SimulatorConfig simulationConfig, ScheduledSimConfig scheduledConfig, int number_of_workers, NodeContainer nodes){
     int64_t rtt_us = 4 * simulationConfig.link_delay_us;

    ApplicationContainer frontend;
    Ptr <ScheduledFrontend> c = CreateObject<ScheduledFrontend>();

    c->SetAttribute("Local", AddressValue(InetSocketAddress(Ipv4Address::GetAny(), 1024)));
    c->SetAttribute("PayloadSize", UintegerValue(simulationConfig.payload_size));
    c->SetAttribute("Workers", UintegerValue(number_of_workers));

    c->expected_rtt = rtt_us;
    c->reference_window = scheduledConfig.reference_window;
    c->safety_factor = scheduledConfig.safety_factor;
    c->rate_tolerance = scheduledConfig.rate_tolerance;
    c->preemptive_cancel = scheduledConfig.preemptive_cancel_count;

    nodes.Get(0)->AddApplication(c);
    frontend.Add(c);

    Simulator::Schedule(NanoSeconds (simulationConfig.sending_start_time_ns), &ScheduledFrontend::SendingLoop, c);
    std::vector<Address> frontendAddress;
    frontendAddress.emplace_back(InetSocketAddress(nodes.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), 1024));

    // Install worker app on each node from 1 to n
    printf("  > Setting up workers \n");

    for (int i = 1; i < number_of_workers + 1; i++) {
        Ptr <ScheduledWorker> scheduledWorker = CreateObject<ScheduledWorker>();

        scheduledWorker->SetFrontends(frontendAddress);
        scheduledWorker->safety = scheduledConfig.safety_factor;
        nodes.Get(i)->AddApplication(scheduledWorker);

        ApplicationContainer app;

        app.Add(scheduledWorker);
        app.Start(NanoSeconds(simulationConfig.simulation_start_time_ns + i * 10000));
    }

    return frontend;
}

void ScatterGatherBase::start_poisson_sending(SimulatorConfig simulationConfig, PoissonConfig poissonConfig, ApplicationContainer frontend, int number_of_workers){
    int n = ceil((double) simulationConfig.payload_size / (10 * 1380));
    int standard_transmission = ceil(((double) 8 * 15000 / simulationConfig.link_data_rate_gigabit_per_s) * (1 + poissonConfig.scheduled_config.safety_factor));
    int leftover = simulationConfig.payload_size - (n - 1) * 10 * 1380;
    int leftover_transmission = ceil((double) 8 * leftover / 1380 * 1500 / simulationConfig.link_data_rate_gigabit_per_s) * (1 + poissonConfig.scheduled_config.safety_factor);

    int fct = ((n-1) * standard_transmission + leftover_transmission) * number_of_workers;

    Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();

    double lambda = ((double) poissonConfig.poisson_rate) / (1*fct);
    double t = 0;

    for (int k = 0; k < poissonConfig.number_requests; ++k) {
        double r = uv->GetValue();

        double start_time = - std::log(1 - r) / lambda;
        t += start_time;

        Simulator::Schedule(NanoSeconds(poissonConfig.incast_base_start + t), &ScheduledFrontend::SendingLoop,
                            (frontend.Get(k))->GetObject<ScheduledFrontend>());
    }
}
}

