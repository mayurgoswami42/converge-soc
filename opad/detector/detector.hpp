#pragma once

#include <unordered_map>
#include <string>
#include <queue>
#include <span>
#include <vector>
#include <chrono>
#include <utility>
#include <sstream>

#include "anomaly_struct.hpp"
#include "utils/utils.hpp"
#include "utils/debug.hpp"

struct TimeNCount
{
    size_t sus_count = 0;
    size_t req_count = 0;
    utils::time prev_time = std::chrono::system_clock::now();
};

class Detector
{
public:
    typedef std::unordered_map<std::string, std::string> Log;
private:
    const size_t buf_size{}, sus_req_limit{};
    const double max_speed{};
    double rate_speed{}, scan_speed{}, error_speed{};
    std::queue<Log> window{};
    std::vector<std::pair<Log, Anomaly>> sus_logs{};
    std::unordered_map<std::string, TimeNCount> ip_frequency{};
    TimeNCount errors{}, not_found{};
    void perform_checks(const size_t &ip_count, const Log &log);
    void window_increment(const Log &log);
    void window_decrement(const Log &log);
    const double req_speed(const utils::time &i_time, const utils::time &f_time) const;
    // both functions do_insert and process are just for efficient switching, keeping them private to force user to use insert

    using InsertFn = void (Detector::*)(const Log&);
    void do_insert(const Log& log); // insert until buf_size of full, both the functions processes the log by default
    void process(const Log& log); // then insert and pop_front, sliding window kind of processing
    
public:
    InsertFn insert = nullptr;
    // max_req_speed is in requests/second
    Detector(const size_t buffer_size, const size_t sus_req_limit, const double max_req_speed) : buf_size(buffer_size), sus_req_limit(sus_req_limit), max_speed(max_req_speed), insert(&Detector::do_insert) {};
    Detector(const std::span<const Log> &logs, const size_t buffer_size, const size_t sus_req_limit, const double max_req_speed);
    bool max_error_rate = false;
    bool ip_attack = false;
    bool directory_attack = false;
    void reset();
    size_t sus_log_counts() const;
    const std::vector<std::pair<Log, Anomaly>> &get_sus_logs() const;
    const std::queue<Log> &get_log_buffer() const;
    const std::string get_speed_snap();
};