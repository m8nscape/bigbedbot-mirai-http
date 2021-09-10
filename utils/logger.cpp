#include <ctime>
#include <iomanip>
#include <mutex>
#include <cstdarg>
#include <cstring>

#include "utils/logger.h"

#ifdef WIN32
const char* BashColorStrs[] =
{
    "", "", "", "", "", "", "", "",
};
#else
const char* BashColorStrs[] = 
{
    [static_cast<unsigned>(BashColors::None)] = "\e[0m",
    [static_cast<unsigned>(BashColors::Red)] = "\e[0;31m",
    [static_cast<unsigned>(BashColors::Green)] = "\e[0;32m",
    [static_cast<unsigned>(BashColors::Yellow)] = "\e[0;33m",
    [static_cast<unsigned>(BashColors::Blue)] = "\e[0;34m",
    [static_cast<unsigned>(BashColors::Purple)] = "\e[0;35m",
    [static_cast<unsigned>(BashColors::Cyan)] = "\e[0;36m",
    [static_cast<unsigned>(BashColors::Gray)] = "\e[0;37m",
};
// vscode color lint fix "
#endif

const BashColors LevelColor[] = 
{
    BashColors::Red,        // LOG_ERROR
    BashColors::Yellow,     // LOG_WARNING
    BashColors::None,       // LOG_INFO
    BashColors::Cyan,       // LOG_DEBUG
    BashColors::Gray,       // LOG_VERBOSE
};

inline std::ostream& getOutStream()
{
    return use_fstream ? logfstream : std::cout;
}

void addLog(LogLevel level, const char* tag, const char* fmt, ...)
{
    if ((int)level > gLogLevel) return;

    static std::mutex m;
    m.lock();
    std::time_t t = std::time(nullptr);
    auto &os = getOutStream();

    BashColors c = BashColors::None;
    if (level < sizeof(LevelColor) / sizeof(BashColors))
        c = LevelColor[level];

    if (!use_fstream)
        os << BashColorStrs[static_cast<unsigned>(c)];

    os << std::put_time(std::localtime(&t), "%c %Z ");
    switch (level)
    {
        case LOG_ERROR:
            os << "ERROR | [" << tag << "] ";
            break;
        case LOG_WARNING:
            os << "WARN  | [" << tag << "] ";
            break;
        case LOG_INFO:
            os << "INFO  | [" << tag << "] ";
            break;
        case LOG_DEBUG:
            os << "DEBUG | [" << tag << "] ";
            break;
        case LOG_VERBOSE:
            os << "VERB  | [" << tag << "] ";
            break;
        default:
            os << "UNKN  | [" << tag << "] ";
            break;
    }

    static char logbuf[4096];
    logbuf[sizeof(logbuf) - 1] = 0;
    va_list args;
    va_start(args, fmt);
    vsnprintf(logbuf, sizeof(logbuf), fmt, args);
    va_end(args);
    os << logbuf;
    
    if (!use_fstream)
        os << BashColorStrs[static_cast<unsigned>(BashColors::None)];

    os << std::endl;
    m.unlock();

}