#include "strutil.h"

std::string strfmt(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::string res = vstrfmt(fmt, args, true);
    va_end(args);
    return res;
}


std::string vstrfmt(const char* fmt, va_list& args, bool log)
{
    // calc buf size
    va_list argTmp;
    va_copy(argTmp, args);
    int bufSize = vsnprintf(NULL, 0, fmt, argTmp) + 1;
    va_end(argTmp);

    std::string res;
    static char buf[2048];
    if (bufSize > sizeof(buf))
    {
        if (log)
        {
            snprintf(buf, sizeof(buf), "Str length is too long: %s", fmt);
            addLog(LOG_WARNING, "strutil", buf);
        }
        
        // alloc from heap
        char* bufHeap = new char[bufSize];
        vsnprintf(bufHeap, bufSize, fmt, args);
        delete[] bufHeap;
        res = bufHeap;
    }
    else
    {
        // print to static buf
        vsnprintf(buf, sizeof(buf), fmt, args);
        res = buf;
    }
    return res;
}