#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <string>
#include <stdexcept>
#include <vector>
#include <fstream>

#include <map>
#include <string.h>
#include <sys/stat.h>


std::string trim(std::string s);

bool file_exists(std::string& filename);

std::vector<std::string> split_string(const std::string line, const std::string delimiter);

bool starts_with(const std::string& str, const std::string& prefix);

bool ends_with(const std::string& str, const std::string& suffix);

std::string remove_start_end_double_quote_if_present(std::string s);

std::string get_param(const std::string& param_key, std::map<std::string, std::string>& config);

int64_t parse_int64(const std::string& str);

double parse_double(const std::string& str);

void read_config_file(std::string& filename, std::map<std::string, std::string>& config);

#endif
