#ifndef RATE_CONFIG_H
#define RATE_CONFIG_H

#include "base_config.h"

class RateConfig: public BaseConfig {
public:
    int64_t bandwidth;

    int64_t cwnd;
    int64_t MSS;
    int64_t MTU;

    double safety_factor;

    int64_t inter_request;

    int64_t chunk_size_per_round;

    RateConfig() : BaseConfig() {}

    std::map<std::string, std::string> load_config (std::string& filename) override{
        std::map<std::string, std::string> config = BaseConfig::load_config(filename);
        
        bandwidth = parse_int64(get_param("bandwidth", config));
        cwnd = parse_int64(get_param("init_cwnd", config));
        MSS = parse_int64(get_param("mss", config));
        MTU = parse_int64(get_param("mtu", config));

        safety_factor = parse_double(get_param("safety_factor", config));

        inter_request = parse_int64(get_param("inter_request", config));

        chunk_size_per_round = parse_int64(get_param("chunk_size_round", config));

        return config;
    }
};


#endif
