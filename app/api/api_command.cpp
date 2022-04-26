#include "api.h"
#include "../data/group.h"
#include "../../mirai/api.h"

namespace bbb::api::command
{
    
using nlohmann::json;

ProcResp post_send_msg_group(std::string_view path, const std::map<std::string, std::string> params, const std::string& body)
{
    json req = json::parse(body);
    if (req.find("group") == req.end())
        return {beast::http::status::ok, R"({"result": 1, "msg": "param 'group' not found"})"};
    if (req.find("text") == req.end())
        return {beast::http::status::ok, R"({"result": 1, "msg": "param 'text' not found"})"};

    int64_t group = req["group"].get<int64_t>();
    std::string text = req["text"].get<std::string>();

    if (::grp::groups.find(group) == ::grp::groups.end())
    {
        char buf[80];
        sprintf(buf, R"({"result": 1, "msg": "group '%ld' not found"})", group);
        return { beast::http::status::ok, buf };
    }

    if (mirai::sendGroupMsgStr(group, text) == 0)
    {
        return { beast::http::status::ok, R"({"result": 0})" };
    }
    else
    {
        return { beast::http::status::ok, R"({"result": 1})" };
    }
}

}