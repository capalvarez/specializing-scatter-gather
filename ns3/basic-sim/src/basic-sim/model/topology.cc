#include "topology.h"

namespace ns3 {

Topology::Topology(const std::string& filename) {
    std::map<std::string, std::string> config = read_config(filename);
    this->num_nodes = parse_positive_int64(get_param_or_fail("num_nodes", config));
    this->num_undirected_edges = parse_positive_int64(get_param_or_fail("num_undirected_edges", config));

    // Node types
    std::string tmp = get_param_or_fail("switches", config);
    this->switches = parse_set_positive_int64(tmp);
    all_items_are_less_than(this->switches, num_nodes);
    tmp = get_param_or_fail("switches_which_are_tors", config);
    this->switches_which_are_tors = parse_set_positive_int64(tmp);
    all_items_are_less_than(this->switches_which_are_tors, num_nodes);
    tmp = get_param_or_fail("servers", config);
    this->servers = parse_set_positive_int64(tmp);
    all_items_are_less_than(this->servers, num_nodes);

    // Adjacency list
    for (int i = 0; i < this->num_nodes; i++) {
        adjacency_list.push_back(std::set<int64_t>());
    }

    // Edges
    tmp = get_param_or_fail("undirected_edges", config);
    std::set<std::string> string_set = parse_set_string(tmp);
    for (std::string s : string_set) {
        std::vector<std::string> spl = split_string(s, "-", 2);
        int64_t a = parse_positive_int64(spl[0]);
        int64_t b = parse_positive_int64(spl[1]);
        if (a == b) {
            throw std::invalid_argument(format_string("Cannot have edge to itself on node %" PRIu64 "", a));
        }
        if (a >= this->num_nodes) {
            throw std::invalid_argument(format_string("Left node identifier in edge does not exist: %" PRIu64 "", a));
        }
        if (b >= this->num_nodes) {
            throw std::invalid_argument(format_string("Right node identifier in edge does not exist: %" PRIu64 "", b));
        }
        undirected_edges.push_back(std::make_pair(a < b ? a : b, a < b ? b : a));
        undirected_edges_set.insert(std::make_pair(a < b ? a : b, a < b ? b : a));
        adjacency_list[a].insert(b);
        adjacency_list[b].insert(a);
    }

    if (undirected_edges.size() != (size_t) num_undirected_edges) {
        throw std::invalid_argument("Indicated number of undirected edges does not match edge set");
    }

    // Node type hierarchy checks

    if (!direct_set_intersection(this->servers, this->switches).empty()) {
        throw std::invalid_argument("Server and switch identifiers are not distinct");
    }

    if (direct_set_union(this->servers, this->switches).size() != (size_t) num_nodes) {
        throw std::invalid_argument("The servers and switches do not encompass all nodes");
    }

    if (direct_set_intersection(this->switches, this->switches_which_are_tors).size() != this->switches_which_are_tors.size()) {
        throw std::invalid_argument("Servers are marked as ToRs");
    }

    // Servers must be connected to ToRs only
    for (int64_t node_id : this->servers) {
        for (int64_t neighbor_id : adjacency_list[node_id]) {
            if (this->switches_which_are_tors.find(neighbor_id) == this->switches_which_are_tors.end()) {
                throw std::invalid_argument(format_string("Server node %" PRId64 " has an edge to node %" PRId64 " which is not a ToR.", node_id, neighbor_id));
            }
        }
    }

    if (this->servers.size() > 0) {
        has_zero_servers = false;
    } else {
        has_zero_servers = true;
    }

}

bool Topology::is_valid_flow_endpoint(int64_t node_id) {
    if (has_zero_servers) {
        return this->switches_which_are_tors.find(node_id) != this->switches_which_are_tors.end();
    } else {
        return this->servers.find(node_id) != this->servers.end();
    }
}

}
