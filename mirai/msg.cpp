#include <sstream>

#include "msg.h"
#include "api.h"

namespace mirai
{

MsgMetadata parseMsgMetadata(const json& v)
{
    MsgMetadata meta;

    if (v.contains("type"))
    {
        const std::string type = v.at("type").get<std::string>();
        if (type == "FriendMessage") meta.source = MsgMetadata::FRIEND;
        else if (type == "TempMessage") meta.source = MsgMetadata::TEMP;
        else if (type == "GroupMessage") meta.source = MsgMetadata::GROUP;
    }

    if (v.contains("messageChain") && v.at("messageChain").contains("Source"))
    {
        const json& s = v.at("messageChain").at("Source");
        meta.msgid = s.at("id").get<int>();
        meta.time = s.at("time").get<int>();
    }

    if (v.contains("sender"))
    {
        const json& s = v.at("sender");
        if (s.contains("id"))
            meta.qqid = s.at("id").get<int>();
        if (s.contains("group"))
        {
            const json& g = s.at("group");
            if (g.contains("id"))
                meta.groupid = g.at("id").get<int>();
        }
    }

    return meta;
}

std::string convertMessageChainObject(const json& e)
{
    if (!e.contains("type")) return "";

    auto type = e.at("type").get<std::string>();
    std::stringstream ss;

    if (type == "Plain" && e.contains("text"))
        ss << e.at("text").get<std::string>();
    else if (type == "At" && e.contains("target"))
        ss << " @" << e.at("target").get<int64_t>() << " ";

    return ss.str();
}
    
std::string messageChainToStr(const json& v)
{
    if (!v.contains("messageChain")) return "";

    const auto& m = v.at("messageChain");
    std::stringstream ss;
    for (const auto& e: m)
    {
        ss << convertMessageChainObject(e);
    }
    return ss.str();
}

std::vector<std::string> messageChainToArgs(const json& v, unsigned max_count)
{
    auto str = messageChainToStr(v);
    auto ss = std::stringstream(str);

    std::vector<std::string> args;
    size_t lengthProcessed = 0;
    while (!ss.eof() && args.size() < max_count - 1)
    {
        std::string s;
        ss >> s;
        args.push_back(s);
        lengthProcessed += s.length();
    }
    if (!ss.eof())
    {
        while (str.length() > lengthProcessed
            && (str[lengthProcessed] == ' ' || str[lengthProcessed] == '\t' || str[lengthProcessed] == '\n'))
            lengthProcessed++;

        args.push_back(str.substr(lengthProcessed));
    }
    return args;
}

int sendMsgRespStr(const MsgMetadata& meta, const std::string& str, int64_t quoteMsgId)
{
    switch (meta.source)
    {
    case MsgMetadata::GROUP:
        return sendGroupMsgStr(meta.groupid, str, quoteMsgId);
    case MsgMetadata::FRIEND:
        return sendFriendMsgStr(meta.qqid, str, quoteMsgId);
    case MsgMetadata::TEMP:
        return sendTempMsgStr(meta.qqid, meta.groupid, str, quoteMsgId);
        break;
    default:
        break;
    }
    return -1;
}

int sendMsgResp(const MsgMetadata& meta, const json& messageChain, int64_t quoteMsgId)
{
    switch (meta.source)
    {
    case MsgMetadata::GROUP:
        return sendGroupMsg(meta.groupid, messageChain, quoteMsgId);
    case MsgMetadata::FRIEND:
        return sendFriendMsg(meta.qqid, messageChain, quoteMsgId);
    case MsgMetadata::TEMP:
        return sendTempMsg(meta.qqid, meta.groupid, messageChain, quoteMsgId);
        break;
    default:
        break;
    }
    return -1;
}

}