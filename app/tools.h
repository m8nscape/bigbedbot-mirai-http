#pragma once
#include <functional>
#include <vector>
#include <string>
#include <map>

#include "nlohmann/json.hpp"

namespace tools
{

enum class commands : size_t {
	roll,

};

using nlohmann::json;
void msgDispatcher(const json& body);
}