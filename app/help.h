#pragma once
#include <string>
#include <vector>
#include <map>
#include <functional>

#include "nlohmann/json.hpp"

namespace help {

using nlohmann::json;

enum class commands : size_t {
	help
};

void msgDispatcher(const json& body);

std::string boot_info();
std::string help();
}