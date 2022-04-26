#pragma once
#include "api.h"

namespace bbb::api::command
{

ProcResp post_send_msg_group(std::string_view path, const std::map<std::string, std::string> params, const std::string& body);

}