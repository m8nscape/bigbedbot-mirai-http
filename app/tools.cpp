#include <sstream>

#include "tools.h"
#include "utils/rand.h"
#include "mirai/api.h"
#include "mirai/msg.h"

namespace tools
{

inline std::map<std::string, commands> commands_str
{
	{"!roll", commands::roll},
	{"/roll", commands::roll},
};

std::string roll(const std::vector<std::string>& args)
{
	int max = 100;
	if (args.size() > 1)
	{
		max = std::strtol(args[1].c_str(), nullptr, 10);
		if (max < 0) max = 100;
	}
	return std::to_string(randInt(0, max));
}

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
    case commands::roll:
        resp = roll(query);
        break;
    default: 
        break;
    }

    if (!resp.empty())
    {
        mirai::sendMsgRespStr(m, resp);
    }

}

}