#pragma once

#include <string>
#include <map>
#include <any>
#include <functional>

#include <nlohmann/json.hpp>

namespace mirai::http
{
void set_port(unsigned short port);

using nlohmann::json;

int connect();
int disconnect();

int GET(const std::string& path, std::function<int(const char*, const json&, const json&)> callback);
int POST(const std::string& path, const json& body, std::function<int(const char*, const json&, const json&)> callback);

}
