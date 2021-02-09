#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include <utility>
#include "simon-util.h"

namespace ns3 {

class Topology
{
public:
    int64_t num_nodes;
    int64_t num_undirected_edges;
    std::set<int64_t> switches;
    std::set<int64_t> switches_which_are_tors;
    std::set<int64_t> servers;
    std::vector<std::pair<int64_t, int64_t>> undirected_edges;
    std::set<std::pair<int64_t, int64_t>> undirected_edges_set;
    std::vector<std::set<int64_t>> adjacency_list;

    Topology(const std::string& filename);
    bool is_valid_flow_endpoint(int64_t node_id);

private:
    bool has_zero_servers;

};

}

#endif //TOPOLOGY_H
