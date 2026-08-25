#pragma once

#include <string>
#include <chrono>
#include <utility>
#include <optional>

namespace utils
{
    typedef std::chrono::system_clock::time_point time;
    typedef std::chrono::duration<double> duration;
    
    std::string time_str();
    std::string date_str();

    std::string stringify(int num);
    std::string stringify(double num);
    std::string stringify(std::string str);

    void thread_sleep(int milliseconds);

    std::optional<utils::time> parse_date_time(const std::string &date, const std::string &time);

    utils::time now();
}