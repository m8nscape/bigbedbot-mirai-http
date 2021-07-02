#pragma once

#include <string>
#include <map>
#include <any>
#include <functional>

namespace mirai::ws
{
void set_port(unsigned short port);

int connect(const std::string& path);
int disconnect();

void setRecvCallback(std::function<void(const std::string& msg)> cb);

}
