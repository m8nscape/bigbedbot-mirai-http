#pragma once

#include <string>

#include "nlohmann/json.hpp"

namespace monopoly
{
using nlohmann::json;
int init(const char* yaml);
void msgCallback(const json& body);
}