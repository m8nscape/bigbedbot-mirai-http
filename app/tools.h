#pragma once
#include <functional>
#include <vector>
#include <string>
#include <map>

#include "nlohmann/json.hpp"

namespace tools
{
using nlohmann::json;
void msgDispatcher(const json& body);
}