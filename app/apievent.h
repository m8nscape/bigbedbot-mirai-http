#pragma once

#include "nlohmann/json.hpp"

namespace apievent
{
using nlohmann::json;
void NewFriendRequestEvent(const json& req);
}