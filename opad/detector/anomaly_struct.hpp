#pragma once

#include <string>
#include <unordered_map>

enum Anomaly // anomaly type
{
    MAX_REQUESTS_FROM_A_IP,
    DIRECTORY_ATTACK,
    SPIKE_ERROR_RATE
};

// definations are in detector.cpp
std::string anomaly_to_string(Anomaly anomaly);