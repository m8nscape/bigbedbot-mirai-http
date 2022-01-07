#pragma once

//#include "private/seniverse-apikey.h"

#include <string>
#include <nlohmann/json.hpp>
#include "mirai/msg.h"

namespace weather
{
using nlohmann::json;
    
void msgCallback(const json& body);

void weather_global(const mirai::MsgMetadata& m, const std::string& city);
void weather_cn(const mirai::MsgMetadata& m, const std::string& city);
void weather_mj(const mirai::MsgMetadata& m, const std::string& city_keyword);

int init(const char* yaml);
}