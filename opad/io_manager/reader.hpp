#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

#include "../utils/debug.hpp"

class Reader
{
public:
    Reader();
    std::vector<std::string> read_logs(std::string);
    void reset_offset();
    bool status = false;
    int get_offset() const;
    void close() const;
    
private:
    static constexpr const char* STATE_FILE = "storage/.reader_offset"; // store offset of log file
    std::streampos offset{};
    std::streampos read_offset() const;
    std::streampos write_offset(std::streampos) const;
    void initialize_storage() const; // creates offset file (STATE_FILE) if dont exists
    bool custom_getline(std::ifstream &ifile, std::string &line); // std::getline is not suitable for our case, see function definition
};