/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef BASIC_SIM_H
#define BASIC_SIM_H

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
#include "ns3/flow-monitor-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/random-variable-stream.h"
#include "ns3/command-line.h"
#include "ns3/simon-util.h"
#include "ns3/schedule-reader.h"
#include "ns3/topology.h"
#include "ns3/flow-send-helper.h"
#include "ns3/flow-send-application.h"

namespace ns3 {
    int basic_sim(std::string run_dir);
}

#endif /* BASIC_SIM_H */
