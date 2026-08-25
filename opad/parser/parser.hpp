#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <any>
#include <utility>

#include <re2/re2.h>

class Parser
{
public:
    // google/re2 patterns are supported only
    // for syntax visit https://github.com/google/re2/wiki/syntax
    Parser(const std::string_view &labels_types, const std::string &regex_pattern); // single pattern logs
    std::unordered_map<std::string, std::string> parse(const std::string &log_line);

private:
    const re2::RE2 regex_obj;
    std::vector<std::pair<std::string, std::string>> parsed_logs;
};