#pragma once

#include <functional>
#include <vector>
#include <string>
#include <map>
#include <cstdint>

#include "nlohmann/json.hpp"

namespace smoke
{
using nlohmann::json;

enum class RetVal
{
    OK,
    ZERO_DURATION,
    TARGET_IS_ADMIN,
    INVALID_DURATION,
    TARGET_NOT_FOUND,
};

RetVal nosmoking(int64_t group, int64_t target, int duration_min);

////////////////////////////////////////////////////////////////////////////////

void groupMsgCallback(const json& body);
void privateMsgCallback(const json& body);
void MemberMuteEvent(const json& body);
void MemberUnmuteEvent(const json& body);

bool isSmoking(int64_t qqid, int64_t groupid);

const double UNSMOKE_COST_RATIO = 3;
}