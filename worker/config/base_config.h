#ifndef BASE_CONFIG_H
#define BASE_CONFIG_H

#include <map>
#include "string_utils.h"

/**
 * Reads and stores the different configuration parameters required by the base worker, see base_example.config.
 */

class BaseConfig {
public:
    int cpu_index;
    std::string socket_address;
    int port;
    char* buffer;

    BaseConfig(){}

    virtual std::map<std::string, std::string> load_config (std::string& filename){
        std::map<std::string, std::string> config;

        read_config_file(filename, config);

        std::string test_file = get_param("test_file_name", config);
        int max_payload = parse_int64(get_param("max_payload", config));
        cpu_index = parse_int64(get_param("cpu_index", config));

        socket_address = get_param("ip_address", config);
        port = parse_int64(get_param("port", config));

        buffer = (char*) malloc(max_payload * sizeof(char));
        std::ifstream infile(test_file);
        infile.read(buffer, max_payload);

        return config;
    }
};


#endif
