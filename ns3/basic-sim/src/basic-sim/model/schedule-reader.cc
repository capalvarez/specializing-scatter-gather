#include "schedule-reader.h"

namespace ns3 {

/**
 * Read the schedule.csv into a schedule.
 *
 * @param filename                  File name of the schedule.csv
 * @param num_nodes                 Total number of nodes present (to check identifiers)
 * @param simulation_end_time_ns    Simulation end time (ns) : all flows must start less than this value
*/
std::vector<schedule_entry_t> read_schedule(const std::string& filename, const int64_t num_nodes, const int64_t simulation_end_time_ns) {

    // Schedule to put in the data
    std::vector<schedule_entry_t> schedule;

    // Check that the file exists
    if (!file_exists(filename)) {
        throw std::runtime_error(format_string("File %s does not exist.", filename.c_str()));
    }

    // Open file
    std::string line;
    std::ifstream schedule_file(filename);
    if (schedule_file) {

        // Go over each line
        size_t line_counter = 0;
        while (getline(schedule_file, line)) {

            // Split on ,
            std::vector<std::string> comma_split = split_string(line, ",", 7);

            // Fill entry
            schedule_entry_t entry = {};
            entry.flow_id = parse_positive_int64(comma_split[0]);
            if (entry.flow_id != (int64_t) line_counter) {
                throw std::invalid_argument(format_string("Flow ID is not ascending by one each line (violation: %" PRId64 ")\n", entry.flow_id));
            }
            entry.from_node_id = parse_positive_int64(comma_split[1]);
            entry.to_node_id = parse_positive_int64(comma_split[2]);
            entry.size_byte = parse_positive_int64(comma_split[3]);
            entry.start_time_ns = parse_positive_int64(comma_split[4]);
            entry.additional_parameters = comma_split[5];
            entry.metadata = comma_split[6];

            // Check node IDs
            if (entry.from_node_id >= num_nodes) {
                throw std::invalid_argument(format_string("Invalid from node ID: %" PRId64 ".", entry.from_node_id));
            }
            if (entry.to_node_id >= num_nodes) {
                throw std::invalid_argument(format_string("Invalid to node ID: %" PRId64 ".", entry.to_node_id));
            }
            if (entry.from_node_id == entry.to_node_id) {
                throw std::invalid_argument(format_string("Flow to itself at node ID: %" PRId64 ".", entry.to_node_id));
            }

            // Check start time
            if (entry.start_time_ns >= simulation_end_time_ns) {
                throw std::invalid_argument(format_string(
                        "Flow %" PRId64 " has invalid start time %" PRId64 " >= %" PRId64 ".",
                        entry.flow_id, entry.start_time_ns, simulation_end_time_ns
                ));
            }

            // Put into schedule
            schedule.push_back(entry);

            // Next line
            line_counter++;

        }

        // Close file
        schedule_file.close();

    } else {
        throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
    }

    return schedule;

}

}
