#ifndef SCHEDULE_READER_H
#define SCHEDULE_READER_H

#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <cstring>
#include <fstream>
#include <cinttypes>
#include <algorithm>
#include <regex>
#include "simon-util.h"

namespace ns3 {

struct schedule_entry_t {
    int64_t flow_id;
    int64_t from_node_id;
    int64_t to_node_id;
    int64_t size_byte;
    int64_t start_time_ns;
    std::string additional_parameters;
    std::string metadata;
};

std::vector<schedule_entry_t> read_schedule(const std::string& filename, const int64_t num_nodes, const int64_t simulation_end_time_ns);

}

#endif //SCHEDULE_READER_H
