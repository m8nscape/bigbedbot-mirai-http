#include <sstream>
#include <algorithm>

#include "help.h"
#include "mirai/api.h"
#include "mirai/msg.h"

namespace help
{

const std::map<std::string, commands> commands_str
{
	{"帮助", commands::help},
	{"幫助", commands::help},
};

void msgDispatcher(const json& body)
{
    auto query = mirai::messageChainToArgs(body);
    if (query.empty()) return;

    auto cmd = query[0];
    if (commands_str.find(cmd) == commands_str.end()) return;

    auto m = mirai::parseMsgMetadata(body);

    std::string resp;
    switch (commands_str.at(cmd))
    {
    case commands::help:
        resp = help();
        break;
    default: 
        break;
    }

    if (!resp.empty())
    {
        mirai::sendMsgRespStr(m, resp);
    }
}

std::string boot_info()
{
    std::stringstream ss;
    ss << "bot活了！";
    ss << help();
	return ss.str();
}

std::string help()
{
    std::stringstream ss;
    ss << "最后更新日期：" << __DATE__ << " " __TIME__ << std::endl <<
        "帮助文档：https://github.com/yaasdf/bigbedbot-mirai-http/blob/master/app/README.md";
    return ss.str();
}
}
