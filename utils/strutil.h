#pragma once

#include <cstring>
#include <cstdarg>
#include <string>
#include "logger.h"

std::string strfmt(const char* fmt, ...);
std::string vstrfmt(const char* fmt, va_list& args, bool log = true);