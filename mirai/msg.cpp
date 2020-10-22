#include <sstream>

#include "msg.h"
#include "api.h"

namespace mirai
{

int parseMsgMeta(const json& v, int& msgid, time_t& time, int64_t& qqid, int64_t& groupid)
{
    msgid = 0;
    time = 0;
    if (v.contains("messageChain") && v.at("messageChain").contains("Source"))
    {
        const json& s = v.at("messageChain").at("Source");
        msgid = s.at("id").get<int>();
        time = s.at("time").get<int>();
    }

    qqid = 0;
    groupid = 0;
    if (v.contains("sender"))
    {
        const json& s = v.at("sender");
        if (s.contains("id"))
            qqid = s.at("id").get<int>();
        if (s.contains("group"))
        {
            const json& g = s.at("group");
            if (g.contains("id"))
                groupid = g.at("id").get<int>();
        }
    }

    return 0;
}

MsgMetadata parseMsgMetadata(const json& v)
{
    MsgMetadata meta;
    parseMsgMeta(v, meta.msgid, meta.time, meta.qqid, meta.groupid);
    return meta;
}
    
std::vector<std::string> messageChainToArgs(const json& v, unsigned max_count)
{
    if (!v.contains("messageChain")) return {};

    const auto& m = v.at("messageChain");
    std::stringstream ss;
    for (const auto& e: m)
    {
        if (e.contains("text"))
            ss << e.at("text").get<std::string>();
    }

    std::vector<std::string> args;
    while (!ss.eof() && args.size() < max_count)
    {
        std::string s;
        ss >> s;
        args.push_back(s);
    }
    if (!ss.eof()) args.push_back(ss.str());
    return args;
}

int sendMsgRespStr(const MsgMetadata& meta, const std::string& str, int64_t quoteMsgId)
{
    if (meta.groupid == 0) 
        return sendPrivateMsgStr(meta.qqid, str, quoteMsgId);
    else 
        return sendGroupMsgStr(meta.groupid, str, quoteMsgId);
}

int sendMsgResp(const MsgMetadata& meta, const json& messageChain, int64_t quoteMsgId)
{
    if (meta.groupid == 0) 
        return sendPrivateMsg(meta.qqid, messageChain, quoteMsgId);
    else 
        return sendGroupMsg(meta.groupid, messageChain, quoteMsgId);
}

}