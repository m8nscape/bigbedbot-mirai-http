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

const BashColors LOG_ERROR = BashColors::Red;
const BashColors LOG_WARNING = BashColors::Yellow;
const BashColors LOG_INFO = BashColors::None;
const BashColors LOG_DEBUG = BashColors::Cyan;

void addLog(BashColors c, const char* tag, const char* fmt, ...);

#ifdef _DEBUG
#define addLogDebug(tag, fmt, ...)
#else
#define addLogDebug(tag, fmt, ...) addLog(LOG_DEBUG, tag, fmt, ##__VA_ARGS__)
#endif
