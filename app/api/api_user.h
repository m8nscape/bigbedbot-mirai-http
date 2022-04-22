#pragma once
#include "api.h"

namespace bbb::api::user
{

ProcResp get_info(std::string_view path, const std::map<std::string, std::string> params, const std::string& body);
ProcResp get_group_info(std::string_view path, const std::map<std::string, std::string> params, const std::string& body);

ProcResp post_give_currency(std::string_view path, const std::map<std::string, std::string> params, const std::string& body);
ProcResp post_take_currency(std::string_view path, const std::map<std::string, std::string> params, const std::string& body);
}