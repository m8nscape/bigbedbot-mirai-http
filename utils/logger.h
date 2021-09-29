#pragma once
#include <iostream>
#include <fstream>

inline bool use_fstream = false;
inline std::ofstream logfstream;

inline int setLogFile(const char* fpath)
{
    use_fstream = true;
    logfstream = std::ofstream(fpath, std::ios::app);
    return 0;
}

enum class BashColors: unsigned
{
    None,
    Red,
    Green,
    Yellow,
    Blue,
    Purple,
    Cyan,
    Gray
};

enum LogLevel: unsigned
{
    LOG_ERROR = 0,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG,
    LOG_VERBOSE,
};
inline int gLogLevel = LOG_INFO;

void addLog(LogLevel c, const char* tag, const char* fmt, ...);

// #ifdef NDEBUG
// #define addLogDebug(tag, fmt, ...)
// #else
#define addLogDebug(tag, fmt, ...) addLog(LOG_DEBUG, tag, "<%s> " fmt, __FUNCTION__, ##__VA_ARGS__)
// #endif
