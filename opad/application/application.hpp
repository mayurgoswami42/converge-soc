#pragma once

#include <iostream>
#include <thread>
#include <atomic>
#include <csignal>

#include "io_manager/reader.hpp"
#include "parser/parser.hpp"
#include "detector/detector.hpp"

#include "utils/debug.hpp"

class Application
{
public:
    Application(int port, double max_req_speed, int sus_request_limit, int window_size = 50000);
    void detection_loop(const std::string &log_path);

private:
    static void signal_handler(int signum); // handle SIGNUM, SIGTERM to safe exit
    void stop(); // signal_handler calls this
    static std::vector<Application *> instances; // to close all the application instances
    utils::time start_time{};
    std::atomic<double> tool_speed{};
    std::atomic<bool> state_running{true};

    Detector detector;
    Reader reader{};
};