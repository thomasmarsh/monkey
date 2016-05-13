#pragma once

#include <spdlog/spdlog.h>

#include <stack>

//#define NO_LOGGING

//#define NO_DEBUG
//#define NO_TRACE

namespace spd = spdlog;

inline std::string LogBasename(const std::string &path) {
    return path.substr(path.find_last_of("/") + 1).substr(0, 12);
}

#define BASE_LOG(fn, raw, ...) \
    do { \
        auto fmt = std::string("|{:>12s}:{:04d}] ") + raw; \
        spd::get("console")->fn(fmt.c_str(), LogBasename(__FILE__), __LINE__, ##__VA_ARGS__); \
    } while (0)

#define LOG_FLUSH() spd::get("console")->flush()

#ifdef NO_LOGGING

#define LOG(...)
#define WARN(...)
#define NO_DEBUG
#define NO_TRACE

#else

#define LOG(...)  BASE_LOG(info, ##__VA_ARGS__)
#define WARN(...) BASE_LOG(warn, ##__VA_ARGS__)

#endif // NO_LOGGING


#ifdef NO_DEBUG
#define DLOG(...)
#else
#define DLOG(...) BASE_LOG(debug, ##__VA_ARGS__)
#endif // NO_DEBUG


#ifdef NO_TRACE
#define TLOG(fmt, ...)
#define TRACE()
#else
#define TLOG(fmt, ...) \
    do { \
        auto indented = gLogContext.getIndent() + fmt; \
        BASE_LOG(trace, indented.c_str(), ##__VA_ARGS__); \
    } while (0)
#define TRACE() Tracer __trace_(__PRETTY_FUNCTION__, __LINE__)
#endif // NO_TRACE

#define ERROR(m, ...) do { \
        BASE_LOG(error, m, ##__VA_ARGS__); \
        spd::get("console")->flush(); \
        abort(); \
    } while (0)

#define SET_LOG_LEVEL(l) spd::get("console")->set_level(spd::level::l)

struct LogContext {
    using Level = spd::level::level_enum;
    using Lock = std::lock_guard<std::mutex>;

    std::mutex mtx;

    // TODO: make thread local
    int trace_indent;
    std::stack<Level> levels;

    LogContext() : trace_indent(0) {}

    std::string getIndent() {
        Lock lock(mtx);
        return std::string(trace_indent*4, ' ');
    }

    void indent() {
        Lock lock(mtx);
        ++trace_indent;
    }

    void dedent() {
        Lock lock(mtx);
        assert(trace_indent > 0);
        --trace_indent;
    }

    void push(Level level) {
        Lock lock(mtx);
        levels.push(spd::get("console")->level());
        spd::get("console")->set_level(level);
    }

    void pop() {
        Lock lock(mtx);
        assert(!levels.empty());
        spd::get("console")->set_level(levels.top());
        levels.pop();
    }
};

extern LogContext gLogContext;

struct ScopedLogLevel {
    ScopedLogLevel(LogContext::Level level) {
        gLogContext.push(level);
    }

    ~ScopedLogLevel() {
        gLogContext.pop();
    }
};

#define SCOPED_LOG(level) ScopedLogLevel __scoped_log(LogContext::Level level)

struct Tracer {
    const char *f;
    int line;

    Tracer(const char *_f, int l) : f(_f), line(l) {
        log("ENTER");
        gLogContext.indent();
    }

    ~Tracer() {
        gLogContext.dedent();
        log("LEAVE");
    }

    void log(const char *label) const {
        BASE_LOG(trace, "{}{}: {}:{}",  gLogContext.getIndent(), label, f, line);
    }
};

