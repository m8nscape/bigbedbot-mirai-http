#include "api.h"
#include "http_conn.h"

#include <sstream>
#include <vector>
#include <map>
#include <future>

#include <magic_enum.hpp>

#include "util.h"
#include "logger.h"


namespace mirai
{
char sessionKey[64]{0};

using nlohmann::json;

int auth(const char* auth_key)
{
	addLog(LOG_INFO, "api", "Authenciating with auth key [%s]...", auth_key);
    json body;
    body["authKey"] = std::string(auth_key);

    std::promise<int> p;
    std::future<int> f = p.get_future();
	conn::POST("/auth", body, 
		[&p](const json& v) -> int
		{
            int resp = 0;
            if (v.contains("code"))
            {
                if ((resp = v.at("code").get<int>()) == 0)
                {
                    // save sessionKey
                    strncpy(sessionKey, v.at("session").get<std::string>().c_str(), sizeof(sessionKey));
                    addLog(LOG_INFO, "api", "Got session key [%s]", sessionKey);
                }
                else
                {
                    resp = v.at("code").get<int>();
                    addLog(LOG_ERROR, "api", "Authenciation failed with code %d", resp);
                }
            }
            else
            {
                resp = -1;
                addLog(LOG_ERROR, "api", "Authenciation failed");
            }

            p.set_value(resp);
			return resp;
		}
    );
    f.wait();
    return f.get();
}

int verify()
{
	addLog(LOG_INFO, "api", "Verifying with session key [%s]...", sessionKey);
    json body;
    body["sessionKey"] = std::string(sessionKey);
    body["qq"] = botLoginQQId;

    std::promise<int> p;
    std::future<int> f = p.get_future();
	conn::POST("/verify", body, 
		[&p](const json& v) -> int
		{
            int resp = 0;
            if (v.contains("code"))
            {
			    resp = v.at("code").get<int>();
            }
            else
            {
                resp = -1;
            }

            if (resp == 0)
            {
                addLog(LOG_INFO, "api", "Verifying succeeded");
            }
            else
            {
                addLog(LOG_ERROR, "api", "Verification failed with code %d", resp);
            }

            p.set_value(resp);
			return resp;
		}
    );
    f.wait();
    return f.get();
}

int sendMsgCallback(const json& v)
{
    if (!v.contains("code"))
    {
        addLog(LOG_WARNING, "api", "msg failed with unknown reason");
        return 1;
    }
    int code = v.at("code");

    std::string msg;
    if (v.contains("msg"))
    {
        v.at("msg").get_to(msg);
    }
    
    if (code != 0)
    {
        // TODO msg
        addLog(LOG_WARNING, "api", "msg failed with code %d: %s", code, msg.c_str());
        return code;
    }

    if (v.contains("messageId"))
    {
        int64_t msgId = v.at("messageId");
        // TODO msgId
        addLogDebug("api", "msg succeeded (id:%lld)", msgId);
    }
    else
    {
        addLogDebug("api", "msg succeeded");
    }

    return 0;
}

int sendPrivateMsgStr(int64_t qqid, const std::string& msg, int64_t quotemsgid)
{
    addLogDebug("api", "Send private msg to %lld: %s", qqid, msg.c_str());
	json req = R"({ "messageChain": [] })"_json;
    json &messageChain = req["messageChain"];
    std::stringstream ss(msg);
    while (!ss.eof())
    {
        std::string buf;
        std::getline(ss, buf);
        if (!ss.eof()) buf += "\n";
        json v;
        v["type"] = "Plain";
        v["text"] = buf;
        messageChain.push_back(v);
    }
    return sendPrivateMsg(qqid, req, quotemsgid);
}

int sendPrivateMsg(int64_t qqid, const json& messageChain, int64_t quotemsgid)
{
    addLogDebug("api", "Send private msg to %lld: (messagechain)", qqid);
    // TODO check if target is friend

    json obj;
    obj["sessionKey"] = std::string(sessionKey);
    obj["qq"] = qqid;
    if (quotemsgid) obj["quote"] = quotemsgid;
    obj["messageChain"] = messageChain.at("messageChain");

    return conn::POST("/sendTempMessage", obj, sendMsgCallback);
}

int sendGroupMsgStr(int64_t groupid, const std::string& msg, int64_t quotemsgid)
{
    addLogDebug("api", "Send group msg to %lld: %s", groupid, msg.c_str());
	json resp = R"({ "messageChain": [] })"_json;
    json &messageChain = resp["messageChain"];
    std::stringstream ss(msg);
    while (!ss.eof())
    {
        std::string buf;
        std::getline(ss, buf);
        if (!ss.eof()) buf += "\n";
        json v;
        v["type"] = "Plain";
        v["text"] = buf;
        messageChain.push_back(v);
    }
    return sendGroupMsg(groupid, resp, quotemsgid);
}

int sendGroupMsg(int64_t groupid, const json& messageChain, int64_t quotemsgid)
{
    addLogDebug("api", "Send group msg to %lld: (messagechain)", groupid);
    json obj;
    obj["sessionKey"] = std::string(sessionKey);
    obj["target"] = groupid;
    if (quotemsgid) obj["quote"] = quotemsgid;
    obj["messageChain"] = messageChain.at("messageChain");

    return conn::POST("/sendGroupMessage", obj, sendMsgCallback);
}

int recallMsg(int64_t msgid)
{
    addLogDebug("api", "Recall msg %lld", msgid);
    json obj;
    obj["sessionKey"] = std::string(sessionKey);
    obj["target"] = msgid;
    int ret = conn::POST("/recall", obj, sendMsgCallback);
    return ret;
}

int mute(int64_t qqid, int64_t groupid, int time_sec)
{
    addLogDebug("api", "Mute %lld @ %lld for %ds", qqid, groupid, time_sec);
    json obj;
    obj["sessionKey"] = std::string(sessionKey);
    obj["target"] = groupid;
    obj["memberId"] = qqid;
    int ret;
    if (time_sec != 0)
    {
        obj["time"] = time_sec;
        int ret = conn::POST("/mute", obj, sendMsgCallback);
    }
    else
    {
        int ret = conn::POST("/unmute", obj, sendMsgCallback);
    }
    return ret;
}


int getGroupMemberInfo(int64_t groupid, int64_t qqid, group_member_info& g)
{
    addLogDebug("api", "Get member info for %lld @ %lld", qqid, groupid);
    g = group_member_info();
    std::stringstream path;
    path << "/memberInfo?sessionKey=" << sessionKey << "&target=" << groupid << "&memberId=" << qqid;

    std::promise<int> p;
    std::future<int> f = p.get_future();
    conn::GET(path.str(), 
        [&](const json& body)
        {
            if (body.empty()) return -1;

            g.qqid = qqid;
            if (body.contains("permission"))
            {
                const auto& p = body.at("permission");
                if (p == "ADMINISTRATOR") g.permission = group_member_permission::ADMINISTRATOR;
                else if (p == "OWNER") g.permission = group_member_permission::OWNER;
            }
            if (body.contains("name"))
                g.nameCard = body.at("name");
            if (body.contains("specialTitle"))
                g.specialTitle = body.at("specialTitle");
            
            p.set_value(0); 
            return 0; 
        }
    );
    f.wait();
    return f.get();
}

std::vector<group_member_info> getGroupMemberList(int64_t groupid)
{
    addLogDebug("api", "Get member list for group %lld", groupid);
    std::stringstream path;
    path << "/memberList?sessionKey=" << sessionKey << "&target=" << groupid;

    std::vector<group_member_info> l;
    std::promise<int> p;
    std::future<int> f = p.get_future();
    conn::GET(path.str(), 
        [&](const json& body)
        {
            for (const auto& m: body)
            {
                group_member_info g;
                if (body.contains("id"))
                    g.qqid = body.at("id");
                if (body.contains("memberName"))
                    g.nameCard = body.at("memberName");
                if (body.contains("permission"))
                {
                    const auto& p = body.at("permission");
                    if (p == "ADMINISTRATOR") g.permission = group_member_permission::ADMINISTRATOR;
                    else if (p == "OWNER") g.permission = group_member_permission::OWNER;
                }

                l.push_back(g);
            }

            p.set_value(0); 
            return 0; 
        }
    );
    f.wait();
    return l;
}


std::map<RecvMsgType, std::vector<MessageProc>> eventCallbacks;

int procRecvMsgEntry(const json& v)
{
    if (!v.contains("type")) return -2;

    std::string type = v.at("type");
    addLogDebug("api", "Recv event %s", type.c_str());

    auto evt = magic_enum::enum_cast<RecvMsgType>(type);
    if (evt.has_value())
    {
        RecvMsgType e = evt.value();
        for (const auto& cb: eventCallbacks[e])
        {
            cb(v);
        }
        return 0;
    }
    else
    {
        return -1;
    }
}

int recvMsgCallback(const json& v)
{
    if (!v.contains("code")) return 1;
    int code = v.at("code");

    std::string msg;
    if (v.contains("msg"))
    {
        v.at("msg").get_to(msg);
    }
    
    if (code != 0)
    {
        // TODO msg
        return code;
    }

    if (v.contains("data"))
    {
        for (const auto& entry: v.at("data"))
        {
            procRecvMsgEntry(entry);
        }
    }

    return 0;
}

int regEventProc(RecvMsgType evt, MessageProc cb)
{
    eventCallbacks[evt].push_back(cb);
    return 0;
}


std::shared_ptr<std::thread> msg_poll_thread = nullptr;

bool pollingRunning = false;
std::promise<int> pollingPromise;
std::future<int> pollingFuture;

bool isPollingRunning() {return pollingRunning;}

void startMsgPoll()
{
    if (pollingRunning) return;
    
    addLog(LOG_INFO, "mirai", "Start polling message");
    pollingRunning = true;
    pollingFuture = pollingPromise.get_future();
    msg_poll_thread = std::make_shared<std::thread>([]
    {
        while (pollingRunning)
        {
            std::stringstream path;
            path << "/fetchMessage?sessionKey=" << sessionKey << "&count=" << POLLING_QUANTITY;
            std::promise<int> p;
            std::future<int> f = p.get_future();
            conn::GET(path.str(), 
                [&p](const json& v) -> int
                {
                    if (v.contains("data"))
                    {
                        for (const auto& m: v.at("data"))
                            procRecvMsgEntry(m);
                    }
                    p.set_value(0); 
                    return 0; 
                });
            f.wait();
            std::this_thread::sleep_for(std::chrono::milliseconds(POLLING_INTERVAL_MS));
        }
        pollingPromise.set_value(0);
    });
    msg_poll_thread->detach();

}

void stopMsgPoll()
{
    if (!pollingRunning) return;

    addLog(LOG_INFO, "mirai", "Stop polling message");
    pollingRunning = false;
    if (msg_poll_thread)
    {
        pollingFuture.get();
        msg_poll_thread = nullptr;
    }
}


}