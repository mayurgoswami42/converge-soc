#include "detector.hpp"

Detector::Detector(const std::span<const Log> &server_logs, const size_t buffer_size, const size_t sus_req_limit, double attack_threshold) : Detector(buffer_size, sus_req_limit, attack_threshold)
{
    // set the insert initially to do_insert, which will switch to process when window is of buffer_size
    insert = &Detector::do_insert;
    for (const Log &log : server_logs) // feed old logs
        (this->*insert)(log);
}

const std::vector<std::pair<Detector::Log, Anomaly>> &Detector::get_sus_logs() const
{
    return sus_logs;
}

const std::queue<Detector::Log> &Detector::get_log_buffer() const
{
    return window;
}

size_t Detector::sus_log_counts() const
{
    return sus_logs.size();
}

const double Detector::req_speed(const utils::time &i_time, const utils::time &f_time) const
{
    std::chrono::milliseconds _time = std::chrono::duration_cast<std::chrono::milliseconds>(f_time - i_time); // nanoseconds -> milliseconds
    if (_time.count() == 0) return max_speed; // requests are very fast so the time is 0 so to count just return the max speed
    double speed = (1/static_cast<double>(_time.count())) * 1000.0;
    if (speed < 0) speed = -speed;
    return speed;
}

const std::string Detector::get_speed_snap() // use for speed monitor on dashboard
{
    std::ostringstream ss;
    ss << utils::stringify("scan_speed");
    ss << ":";
    ss << utils::stringify(scan_speed);
    ss << ",";
    ss << utils::stringify("error_speed");
    ss << ":";
    ss << utils::stringify(error_speed);
    ss << ",";
    ss << utils::stringify("rate_speed");
    ss << ":";
    ss << utils::stringify(rate_speed);

    // reset as if server traffic stops the method will send old same speeds continuously
    scan_speed = 0.0;
    error_speed = 0.0;
    rate_speed = 0.0;

    return ss.str();
}


// we are not calculating the speed by count/(last sus log time - first sus log time)
// because if first request and the second request has the gap and then attack starts then count/(delay + last - second) this could decrease the speed
// and the first request padding can save the attack from being flagged
// hence we are calculating the speed for each request
void Detector::window_increment(const Log &log)
{
    // log time of current log
    std::optional<utils::time> return_value = utils::parse_date_time(log.at("date"), log.at("time")); // std::string to std::chrono::system_clock::time_point
    if (!return_value.has_value())
    {
        DEBUG_PRINT("DETECTOR::ERROR - Incorrect date or/and time format!");
        return;
    }

    utils::time log_time = return_value.value();

    TimeNCount &ip = ip_frequency[log.at("user_ip")];

    // arguments order matters initial first and final second
    rate_speed = req_speed(ip.prev_time, log_time);

    if (rate_speed >= max_speed) // if requests are faster than the max_speed
        ip.sus_count++; // increment the log count of ip

    ip.req_count++;    
    ip.prev_time = log_time;

    // frequency count for error spike
    if (log.at("log_type") == "ERROR")
    {
        error_speed = req_speed(errors.prev_time, log_time);
        if (error_speed >= max_speed)
            errors.sus_count++;
        errors.req_count++;    
        errors.prev_time = log_time;
    }

    // frequency count for directory attack
    if (log.at("status") == "404")
    {
        scan_speed = req_speed(not_found.prev_time, log_time);
        if (scan_speed >= max_speed)
            not_found.sus_count++;
        not_found.req_count++;
        not_found.prev_time = log_time;
    }

    window.emplace(log);

    perform_checks(ip.sus_count, log); // if calculated variables hits constraints then flag
}

// decreasing window has nothing to do with the time variables (left untouched intentionally)
// here we play a trick to detect the sus logs faster, we dont decrease the sus_count directly we treat all the normal family of sus count as observed
// if the matching log appears again with fast speed then the sus_logs are not decreased until the all logs of same kind are removed
// so even if we get short brust after a delay the sus_count + new_brust will trigger the detector and the anomaly gets detected early
void Detector::window_decrement(const Log &log)
{
    if (ip_frequency.find(log.at("user_ip")) != ip_frequency.end()) // reset may erased the entry already
    {
        TimeNCount &ip = ip_frequency[log.at("user_ip")];
        if (--ip.req_count < 1) // if log with same ip appears we will add it back, removing to save space, may same ip never comes back
            ip_frequency.erase(log.at("user_ip"));
        if (ip.sus_count > ip.req_count) ip.sus_count = ip.req_count;
    }

    // decreasing frequency
    errors.req_count -= log.at("log_type") == "ERROR";
    if (errors.sus_count > errors.req_count) errors.sus_count = errors.req_count;
    not_found.req_count -= log.at("status") == "404";
    if (not_found.sus_count > not_found.req_count) not_found.sus_count = not_found.req_count;

    window.pop();
}

// reset flags, containers after the submitting report
void Detector::reset()
{
    std::queue<Log> empty;
    std::swap(window, empty); // clear window queue fast

    ip_attack = false;
    ip_frequency.clear();
    sus_logs.clear();

    max_error_rate = false;
    errors.sus_count = 0;
    errors.req_count = 0;

    directory_attack = false;
    not_found.sus_count = 0;
    not_found.req_count = 0;

    // switches back to the initial insert to fill the queue upto buffer size
    insert = &Detector::do_insert;
}

// the flags are only reset when the anomaly is reported, else immediate increase and decrease can let slip the alert
void Detector::perform_checks(const size_t &ip_count, const Log& log)
{
    if (ip_count >= sus_req_limit)
    {
        ip_attack = true;
        sus_logs.emplace_back(log, Anomaly::MAX_REQUESTS_FROM_A_IP);
    }
    
    if (errors.sus_count >= sus_req_limit)
    {
        max_error_rate = true;
        sus_logs.emplace_back(log, Anomaly::SPIKE_ERROR_RATE);
    }
    
    if (not_found.sus_count >= sus_req_limit)
    {
        directory_attack = true;
        sus_logs.emplace_back(log, Anomaly::DIRECTORY_ATTACK);
    }
}

// fill the window queue until the buf_size is filled, performs checks too for anomaly
void Detector::do_insert(const Log &log)
{
    window_increment(log);
    
    // its a state machine via function pointer trick
    if (window.size() >= buf_size) insert = &Detector::process;
}

// sliding window method for processing window
void Detector::process(const Log &log)
{
    // first decrement and then increment so that window's max size stays buf_size
    window_decrement(window.front());
    window_increment(log);
}

// declaration is in anomaly_struct.hpp
std::string anomaly_to_string(Anomaly anomaly)
{
    if (anomaly == Anomaly::MAX_REQUESTS_FROM_A_IP)
        return "MAX_REQUESTS_FROM_A_IP";
    if (anomaly == Anomaly::DIRECTORY_ATTACK)
        return "DIRECTORY_ATTACK";
    if (anomaly == Anomaly::SPIKE_ERROR_RATE)
        return "SPIKE_ERROR_RATE";
    return "";
}