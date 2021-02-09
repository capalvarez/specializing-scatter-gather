#ifndef SCHEDULED_SIMULATION_H
#define SCHEDULED_SIMULATION_H

#include <chrono>
#include <iostream>
#include <fstream>
#include <dirent.h>

#include "ns3/simulator.h"
#include "ns3/scheduled-worker.h"
#include "ns3/scheduled-frontend.h"
#include "ns3/scheduled-worker-helper.h"
#include "ns3/incast-timestamp.h"
#include "ns3/batch-info.h"
#include "ns3/scheduling-info.h"

#include "ns3/flow-monitor-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"

#include "simon-util.h"
#include "ns3/scatter-gather-base.h"

namespace ns3{
    int scheduled_simulation(std::string run_dir);
}

#endif
