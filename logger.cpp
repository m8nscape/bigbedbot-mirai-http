#include <ctime>
#include <iomanip>
#include <mutex>
#include <cstdarg>

#include "logger.h"

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

inline std::ostream& getOutStream()
{
    return use_fstream ? logfstream : std::cout;
}

void addLog(BashColors c, const char* tag, const char* fmt, ...)
{
    static std::mutex m;
    m.lock();
    std::time_t t = std::time(nullptr);
    auto &os = getOutStream();

    if (!use_fstream)
        os << BashColorStrs[static_cast<unsigned>(c)];

    switch (c)
    {
        case LOG_ERROR:
            os << std::put_time(std::localtime(&t), "%c %Z ") << "[ERROR] [" << tag << "] ";
            break;
        case LOG_WARNING:
            os << std::put_time(std::localtime(&t), "%c %Z ") << "[WARNING] [" << tag << "] ";
            break;
        case LOG_INFO:
            os << std::put_time(std::localtime(&t), "%c %Z ") << "[INFO] [" << tag << "] ";
            break;
        case LOG_DEBUG:
            os << std::put_time(std::localtime(&t), "%c %Z ") << "[DEBUG] [" << tag << "] ";
            break;
        default:
            os << std::put_time(std::localtime(&t), "%c %Z ") << "[" << tag << "] ";
            break;
    }

    char logbuf[1024]{0};
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