#pragma once

#include <spdlog/spdlog.h>

namespace spd = spdlog;

inline std::string LogBasename(const std::string &path) {
    return path.substr(path.find_last_of("/") + 1).substr(0, 12);
}



#define BASE_LOG(fn, raw, ...) \
    do { \
        auto fmt = std::string("|{:>12s}:{:04d}] ") + raw; \
        spd::get("console")->fn(fmt.c_str(), LogBasename(__FILE__), __LINE__, ##__VA_ARGS__); \
    } while (0)

#define LOG(...) //BASE_LOG(info, ##__VA_ARGS__)
#define WARN(...) //BASE_LOG(warn, ##__VA_ARGS__)
//#define DLOG(...) BASE_LOG(debug, ##__VA_ARGS__)
#define DLOG(...)

//#define TLOG(fmt, ...) do { auto indented = Tracer::indent() + fmt; BASE_LOG(trace, indented.c_str(), ##__VA_ARGS__); } while (0)
#define TLOG(...)

#define ERROR(m, ...) do { \
        BASE_LOG(error, m, ##__VA_ARGS__); \
        spd::get("console")->flush(); \
        abort(); \
    } while (0)

#define SET_LOG_LEVEL(l) spd::get("console")->set_level(spd::level::l)

extern int gTraceLevel;


struct Tracer {
    const char *f;
    int line;

    Tracer(const char *_f, int l) : f(_f), line(l) {
        log("ENTER");
        ++gTraceLevel;
    }

    ~Tracer() {
        assert(gTraceLevel > 0);
        --gTraceLevel;
        log("LEAVE");
    }

    static std::string indent() {
        return std::string(gTraceLevel*4, ' ');
    }

    void log(const char *label) const {
        BASE_LOG(trace, "{}{}: {}:{}",  indent(), label, f, line);
    }
};

//#define TRACE() Tracer __trace_(__PRETTY_FUNCTION__, __LINE__)
#define TRACE()
