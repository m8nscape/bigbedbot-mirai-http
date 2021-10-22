#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <climits>
#include "nlohmann/json.hpp"
#include "utils/logger.h"

namespace mirai
{
using nlohmann::json;

struct MsgMetadata
{
    int     msgid = 0;
    time_t  time = 0;
    int64_t qqid = 0;
    int64_t groupid = 0;    // optional
    enum {
        FRIEND,
        TEMP,
        GROUP
    } source = FRIEND;
};

//int parseMsgMeta(const json&, int& msgid, time_t& time, int64_t& qqid, int64_t& groupid, int& source);
MsgMetadata parseMsgMetadata(const json& v);
std::pair<int64_t, int64_t> parseIdFromGroupEvent(const json& reqEvent);

std::string messageChainToStr(const json&);
std::vector<std::string> messageChainToArgs(const json&, unsigned max_count = UINT_MAX);

int sendMsgRespStr(const MsgMetadata& meta, const std::string& str, int64_t quoteMsgId = 0);
int sendMsgResp(const MsgMetadata& meta, const json& messageChain, int64_t quoteMsgId = 0);

inline const json MSG_TEMPLATE = R"({ "messageChain": [] })"_json;
#define MSG_LINE(x) json::parse( R"({ "messageChain": [{"type":"Plain","text":")" x R"("}] })" )

inline json buildMessagePlain(const std::string& s)
{
    json v;
    try 
    {
        v["type"] = "Plain";
        v["text"] = s;
        return v;
    }
    catch (std::exception& e)
    {
        addLog(LOG_ERROR, "json", "buildMessagePlain exception: %s", e.what());
        addLog(LOG_ERROR, "json", "Content: %s", s.c_str());
        throw e;
    }
}

inline json buildMessageAt(int64_t qqid)
{
    json v;
    try
    {
        v["type"] = "At";
        v["target"] = qqid;
        return v;
    }
    catch (std::exception& e)
    {
        addLog(LOG_ERROR, "json", "buildMessageAt exception: %s", e.what());
        addLog(LOG_ERROR, "json", "target: %ld", qqid);
        throw e;
    }
}

inline json buildMessageAtAll()
{
    json v;
    try
    {
        v["type"] = "AtAll";
        return v;
    }
    catch (std::exception& e)
    {
        addLog(LOG_ERROR, "json", "buildMessageAtAll exception: %s", e.what());
        throw e;
    }
}

}