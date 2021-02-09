#ifndef BACKGROUND_CONFIG_H
#define BACKGROUND_CONFIG_H

#include "base_config.h"

class BackgroundConfig: public BaseConfig {
public:
    int bytes_to_send;

    BackgroundConfig() : BaseConfig() {}

    std::map<std::string, std::string> load_config (std::string& filename) override{
        std::map<std::string, std::string> config = BaseConfig::load_config(filename);

        bytes_to_send = parse_int64(get_param("bytes_to_send", config));

        return config;
    }
};

#endif