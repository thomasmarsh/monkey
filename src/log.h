#pragma once

#include <spdlog/spdlog.h>

namespace spd = spdlog;

inline std::string LogBasename(const std::string &path) {
    return path.substr(path.find_last_of("/") + 1).substr(0, 12);
}

#define BASE_LOG(fn, m, ...) \
    spd::get("console")->fn("|{:>12s}:{:04d}] " m, LogBasename(__FILE__), __LINE__, ##__VA_ARGS__)

#define LOG(m, ...) BASE_LOG(info, m, ##__VA_ARGS__)

#define WARN(m, ...) BASE_LOG(warn, m, ##__VA_ARGS__)

#define ERROR(m, ...) do { \
        BASE_LOG(error, m, ##__VA_ARGS__); \
        spd::get("console")->flush(); \
        abort(); \
    } while (0)

#define SET_LOG_LEVEL(l) spd::get("console")->set_level(spd::level::l)
