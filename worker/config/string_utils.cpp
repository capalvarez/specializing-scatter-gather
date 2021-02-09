#include "string_utils.h"

std::string trim(std::string s){
    s.erase(s.find_last_not_of(" \t\n\r\f\v") + 1);
    s.erase(0, s.find_first_not_of(" \t\n\r\f\v"));

    return s;
}

bool file_exists(std::string& filename){
    struct stat st = {0};
    if (stat(filename.c_str(), &st) == 0) {
        return S_ISREG(st.st_mode);
    } else {
        return false;
    }
}

std::vector<std::string> split_string(const std::string line, const std::string delimiter) {
    // Split by delimiter
    char *cline = (char*) line.c_str();
    char * pch;
    pch = strsep(&cline, delimiter.c_str());
    size_t i = 0;
    std::vector<std::string> the_split;
    while (pch != nullptr) {
        the_split.emplace_back(pch);
        pch = strsep(&cline, delimiter.c_str());
        i++;
    }

    return the_split;
}

bool starts_with(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}

bool ends_with(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

std::string remove_start_end_double_quote_if_present(std::string s) {
    if (s.size() >= 2 && starts_with(s, "\"") && ends_with(s, "\"")) {
        return s.substr(1, s.size() - 2);
    } else {
        return s;
    }
}

std::string get_param(const std::string& param_key, std::map<std::string, std::string>& config){
    if (config.find(param_key) != config.end()) {
        return config[param_key];
    }

    return "";
}

int64_t parse_int64(const std::string& str){
    if (!str.empty()){
        return std::stoll(str);
    }

    return 0;
}

double parse_double(const std::string& str){
    return std::stod(str);
}

void read_config_file(std::string& filename, std::map<std::string, std::string>& config){
    // Check that the file exists
    if (!file_exists(filename)) {
        throw std::runtime_error("Config file does not exist.");
    }

    // Open file
    std::string line;
    std::ifstream config_file(filename);
    if (config_file) {

        // Go over each line
        while (getline(config_file, line)) {

            // Skip commented lines
            if (trim(line).empty() || line.c_str()[0] == '#') {
                continue;
            }

            // Split on =
            std::vector<std::string> equals_split = split_string(line, "=");

            // Save into config
            config[equals_split[0]] = remove_start_end_double_quote_if_present(equals_split[1]);

        }
        config_file.close();
    } else {
        throw std::runtime_error("Config file could not be read");
    }

}

