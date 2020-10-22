#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <climits>
#include "nlohmann/json.hpp"

namespace mirai
{
using nlohmann::json;

struct MsgMetadata
{
    int     msgid = 0;
    time_t  time = 0;
    int64_t qqid = 0;
    int64_t groupid = 0;    // optional
};

int parseMsgMeta(const json&, int& msgid, time_t& time, int64_t& qqid, int64_t& groupid);
MsgMetadata parseMsgMetadata(const json& v);

std::vector<std::string> messageChainToArgs(const json&, unsigned max_count = UINT_MAX);

int sendMsgRespStr(const MsgMetadata& meta, const std::string& str, int64_t quoteMsgId = 0);
int sendMsgResp(const MsgMetadata& meta, const json& messageChain, int64_t quoteMsgId = 0);

inline json buildMessagePlain(const std::string& s)
{
    json v;
    v["type"] = "Plain";
    v["text"] = s;
    return std::move(v);
}

inline json buildMessageAt(int64_t qqid)
{
    json v;
    v["type"] = "At";
    v["target"] = qqid;
    return std::move(v);
}

inline json buildMessageAtAll()
{
    json v;
    v["type"] = "AtAll";
    return std::move(v);
}

}