#include "pch.h"
#include "api.h"
#include "msg.h"
#include "http_conn.h"
#include "ws_conn.h"

#ifdef NDEBUG
#else
#include <functional>
#endif

#include "util.h"
#include "utils/logger.h"


namespace mirai
{
using nlohmann::json;

int testConnection()
{
    std::promise<int> p;
    std::future<int> f = p.get_future();
	http::GET("/about",
		[&p](const char* t, const json& b, const json& v) -> int
		{
            int resp = 0;
            if (v.contains("code"))
            {
                if ((resp = v.at("code").get<int>()) == 0)
                {
                    resp = 0;
                }
                else
                {
                    resp = v.at("code").get<int>();
                }
            }
            else
            {
                resp = -1;
            }

            p.set_value(resp);
			return resp;
		}
    );
    f.wait();
    return f.get();
}

std::string authKey = "auth_key_stub";
std::string sessionKey = "session_key_stub";

void setAuthKey(const std::string& auth_key)
{
    authKey = auth_key;
}

std::string getSessionKey()
{
    return sessionKey;
}

#ifdef NDEBUG

int auth()
{
    const char* auth_key = authKey.c_str();

	addLog(LOG_INFO, "api", "Authenciating with auth key [%s]...", auth_key);
    json body;
    body["authKey"] = std::string(auth_key);

    std::promise<int> p;
    std::future<int> f = p.get_future();
	http::POST("/auth", body, 
		[&p](const char* t, const json& b, const json& v) -> int
		{
            int resp = 0;
            if (v.contains("code"))
            {
                if ((resp = v.at("code").get<int>()) == 0)
                {
                    // save sessionKey
                    sessionKey = v.at("session").get<std::string>();
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
	addLog(LOG_INFO, "api", "Verifying with session key [%s]...", sessionKey.c_str());
    json body;
    body["sessionKey"] = getSessionKey();
    body["qq"] = botLoginQQId;

    std::promise<int> p;
    std::future<int> f = p.get_future();
	http::POST("/verify", body, 
		[&p](const char* t, const json& b, const json& v) -> int
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

#else

int auth()
{
    addLog(LOG_INFO, "api", "auth stub");
    return 0;
}

int verify()
{
	addLog(LOG_INFO, "api", "verify stub");
    return 0;
}

#endif

int registerApp()
{
    if (mirai::auth() != 0)
    {
        addLog(LOG_ERROR, "api", "Auth failed!");
        return -1;
    }

    if (mirai::verify() != 0)
    {
        addLog(LOG_ERROR, "api", "Verify failed!");
        return -2;
    }

    return 0;
}

std::string messageChainToPrintStr(const json& m)
{
    if (!m.contains("messageChain")) return "";

    const json& mc = m.at("messageChain");
    if (mc.empty()) return "";

    const json& b = mc.at(0);
    if (!b.contains("type")) return "";
    if (b.at("type") != "Plain") return "(messageChain)";

    std::string s = b.at("text");
    s = s.substr(0, s.find_first_of('\n'));
    if (mc.size() > 1) 
        return s + "...";
    else 
        return s;
}

int sendMsgCallback(const char* target, const json& body, const json& v)
{
    if (!v.contains("code"))
    {
        addLog(LOG_WARNING, "api", "msg failed with unknown reason");
        addLog(LOG_WARNING, "api", "\nSend: %s\n%s", target ? target : "", body.empty() ? body.dump().c_str() : "");
        addLog(LOG_WARNING, "api", "\nRecv:\n%s", v.dump().c_str());
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
        addLog(LOG_WARNING, "api", "msg failed with code %d: %s", code, msg.c_str());
        addLog(LOG_WARNING, "api", "\nSend: %s\n%s", target ? target : "", body.empty() ? body.dump().c_str() : "");
        addLog(LOG_WARNING, "api", "\nRecv:\n%s", v.dump().c_str());
        return code;
    }

    if (v.contains("messageId"))
    {
        int64_t msgId = v.at("messageId");
        addLogDebug("api", "msg succeeded (id:%lld)", msgId);
    }
    else
    {
        addLogDebug("api", "msg succeeded");
    }

    return 0;
}

int sendTempMsgStr(int64_t qqid, int64_t groupid, const std::string& msg, int64_t quotemsgid)
{
	json req = R"({ "messageChain": [] })"_json;
    json &messageChain = req["messageChain"];
    std::stringstream ss(msg);
    while (!ss.eof())
    {
        std::string buf;
        std::getline(ss, buf);
        if (!ss.eof()) buf += "\n";
        messageChain.push_back(buildMessagePlain(buf));
    }
    return sendTempMsg(qqid, groupid, req, quotemsgid);
}

int sendTempMsg(int64_t qqid, int64_t groupid, const json& messageChain, int64_t quotemsgid)
{
    addLogDebug("api", "Send temp msg to %lld @ %lld: %s", qqid, groupid, messageChainToPrintStr(messageChain).c_str());

    json obj;
    obj["sessionKey"] = getSessionKey();
    obj["qq"] = qqid;
    obj["group"] = groupid;
    if (quotemsgid) obj["quote"] = quotemsgid;
    obj["messageChain"] = messageChain.at("messageChain");

    return http::POST("/sendTempMessage", obj, sendMsgCallback);
}

int sendFriendMsgStr(int64_t qqid, const std::string& msg, int64_t quotemsgid)
{
	json req = R"({ "messageChain": [] })"_json;
    json &messageChain = req["messageChain"];
    std::stringstream ss(msg);
    while (!ss.eof())
    {
        std::string buf;
        std::getline(ss, buf);
        if (!ss.eof()) buf += "\n";
        messageChain.push_back(buildMessagePlain(buf));
    }
    return sendFriendMsg(qqid, req, quotemsgid);
}

int sendFriendMsg(int64_t qqid, const json& messageChain, int64_t quotemsgid)
{
    addLogDebug("api", "Send private msg to %lld: %s", qqid, messageChainToPrintStr(messageChain).c_str());

    json obj;
    obj["sessionKey"] = getSessionKey();
    obj["target"] = qqid;
    if (quotemsgid) obj["quote"] = quotemsgid;
    obj["messageChain"] = messageChain.at("messageChain");

    if (!obj["messageChain"].empty())
    {
        size_t lastMsgIdx = obj["messageChain"].size() - 1;
        auto& text = obj["messageChain"][lastMsgIdx]["text"];
        while (!text.empty() && *text.rbegin() == '\n')
            text.erase(text.size() - 1);
        if (text.empty()) return -1;
    }
    
    return http::POST("/sendFriendMessage", obj, sendMsgCallback);
}

int sendGroupMsgStr(int64_t groupid, const std::string& msg, int64_t quotemsgid)
{
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
    addLogDebug("api", "Send group msg to %lld: %s", groupid, messageChainToPrintStr(messageChain).c_str());
    json obj;
    obj["sessionKey"] = getSessionKey();
    obj["target"] = groupid;
    if (quotemsgid) obj["quote"] = quotemsgid;
    obj["messageChain"] = messageChain.at("messageChain");

    if (!obj["messageChain"].empty())
    {
        size_t lastMsgIdx = obj["messageChain"].size() - 1;
        auto& text = obj["messageChain"][lastMsgIdx]["text"];
        while (!text.empty() && *text.rbegin() == '\n')
            text.erase(text.size() - 1);
        if (text.empty()) return -1;
    }
        
    return http::POST("/sendGroupMessage", obj, sendMsgCallback);
}

int recallMsg(int64_t msgid)
{
    addLogDebug("api", "Recall msg %lld", msgid);
    json obj;
    obj["sessionKey"] = getSessionKey();
    obj["target"] = msgid;
    int ret = http::POST("/recall", obj, sendMsgCallback);
    return ret;
}

int mute(int64_t qqid, int64_t groupid, int time_sec)
{
    addLogDebug("api", "Mute %lld @ %lld for %ds", qqid, groupid, time_sec);
    json obj;
    obj["sessionKey"] = getSessionKey();
    obj["target"] = groupid;
    obj["memberId"] = qqid;
    int ret;
    if (time_sec != 0)
    {
        obj["time"] = time_sec;
        int ret = http::POST("/mute", obj, sendMsgCallback);
    }
    else
    {
        int ret = http::POST("/unmute", obj, sendMsgCallback);
    }
    return ret;
}

int sendEventResp(const std::string& path, const json& req, int operation, const std::string& message)
{
    json obj = req;
    obj["sessionKey"] = getSessionKey();
    obj["eventId"] = req.at("eventId");
    obj["fromId"] = req.at("fromId");
    obj["groupId"] = req.at("groupId");
    obj["operate"] = operation;
    obj["message"] = message;
    return http::POST(path, obj, sendMsgCallback);
}
int respNewFriendRequestEvent(const json& req, int operation, const std::string& message)
{
    addLogDebug("api", "Send resp_newFriendRequestEvent: %d", operation);
    return sendEventResp("/resp/newFriendRequestEvent", req, operation, message);
}
int respMemberJoinRequestEvent(const json& req, int operation, const std::string& message)
{
    addLogDebug("api", "Send resp_memberJoinRequestEvent: %d", operation);
    return sendEventResp("/resp/memberJoinRequestEvent", req, operation, message);
}
int respBotInvitedJoinGroupRequestEvent(const json& req, int operation, const std::string& message)
{
    addLogDebug("api", "Send resp_botInvitedJoinGroupRequestEvent: %d", operation);
    return sendEventResp("/resp/botInvitedJoinGroupRequestEvent", req, operation, message);
}

group_member_info getGroupMemberInfo(int64_t groupid, int64_t qqid)
{
    addLogDebug("api", "Get member info for %lld @ %lld", qqid, groupid);
    auto g = group_member_info();
    std::stringstream path;
    path << "/memberInfo?sessionKey=" << getSessionKey() << "&target=" << groupid << "&memberId=" << qqid;

    std::promise<int> p;
    std::future<int> f = p.get_future();
    http::GET(path.str(), 
        [&](const char* t, const json& b, const json& body)
        {
            if (body.empty()) return -1;

            g.qqid = qqid;
            // if (body.contains("permission"))
            // {
            //     const auto& p = body.at("permission");
            //     if (p == "ADMINISTRATOR") g.permission = group_member_permission::ADMINISTRATOR;
            //     else if (p == "OWNER") g.permission = group_member_permission::OWNER;
            // }
            if (body.contains("name"))
                g.nameCard = body.at("name");
            if (body.contains("specialTitle"))
                g.specialTitle = body.at("specialTitle");
            
            p.set_value(0); 
            return 0; 
        }
    );
    f.wait();
    return g;
}

std::vector<group_member_info> getGroupMemberList(int64_t groupid)
{
    addLogDebug("api", "Get member list for group %lld", groupid);
    std::stringstream path;
    path << "/memberList?sessionKey=" << getSessionKey() << "&target=" << groupid;

    std::vector<group_member_info> l;
    std::promise<int> p;
    std::future<int> f = p.get_future();
    http::GET(path.str(), 
        [&](const char* t, const json& b, const json& body)
        {
            for (const auto& m: body)
            {
                // Add the bot itself to the list, since the list API returns does not contain invoker.
                bool isInvokerPermissionAdded = false;
                if (!isInvokerPermissionAdded)
                {
                    if (m.contains("group"))
                    {
                        const auto& gm = m.at("group");
                        if (gm.contains("permission"))
                        {
                            group_member_info gbot;
                            gbot.qqid = botLoginQQId;
                            gbot.nameCard = "";    // TODO set bot namecard
                            const auto& p = gm.at("permission");
                            if (p == "ADMINISTRATOR") 
                                gbot.permission = group_member_permission::ADMINISTRATOR;
                            else if (p == "OWNER") 
                                gbot.permission = group_member_permission::OWNER;
                            else 
                                gbot.permission = group_member_permission::MEMBER;

                            l.push_back(gbot);
                            isInvokerPermissionAdded = true;
                        }
                    }
                }

                // add other members
                group_member_info g;
                if (m.contains("id"))
                    g.qqid = m.at("id");
                if (m.contains("memberName"))
                    g.nameCard = m.at("memberName");
                if (m.contains("permission"))
                {
                    const auto& p = m.at("permission");
                    if (p == "ADMINISTRATOR") 
                        g.permission = group_member_permission::ADMINISTRATOR;
                    else if (p == "OWNER") 
                        g.permission = group_member_permission::OWNER;
                    else 
                        g.permission = group_member_permission::MEMBER;
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

    if (RecvMsgTypeMap.find(type) != RecvMsgTypeMap.end())
    {
        RecvMsgType e = RecvMsgTypeMap.at(type);
        for (auto& [key, callback]: eventCallbacks[e])
        {
            try
            {
                callback(v);
            }
            catch (std::exception &e)
            {
                std::stringstream ss;
                ss << "Exception occurred at callback[" << key << "] of " << type.c_str() << ": " << e.what() << std::endl;
                ss << "----------Message Begin----------" << std::endl;
                ss << v.dump() << std::endl;
                ss << "----------Message End------------" << std::endl;
                addLog(LOG_ERROR, "api", ss.str().c_str());

                if (rootQQId != 0)
                    sendFriendMsgStr(rootQQId, ss.str());
            }
        }
        return 0;
    }
    else
    {
        return -1;
    }
}

int regEventProc(RecvMsgType evt, MessageProc cb)
{
    eventCallbacks[evt].push_back(cb);
    return 0;
}

#ifdef NDEBUG

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
            path << "/fetchMessage?sessionKey=" << getSessionKey() << "&count=" << POLLING_QUANTITY;
            std::promise<int> p;
            std::future<int> f = p.get_future();
            http::GET(path.str(), 
                [&p](const char* t, const json& b, const json& v) -> int
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

void connectMsgWebSocket()
{
    std::stringstream path;
    path << "/all?sessionKey=" << getSessionKey();
    ws::setRecvCallback([](const std::string& msg)
    {
        procRecvMsgEntry(json::parse(msg));
    });

    ws::connect(path.str());
}

void disconnectMsgWebSocket()
{
    ws::disconnect();
}

#else

enum class ConsoleState
{
    EXIT,
    CMD,
    PRIVATE,
    TEMP,
    GROUP
};

void consoleInputHandle_CMD(const std::string& s);
ConsoleState fsmState = ConsoleState::CMD;
std::function<void(const std::string&)> cb = std::bind(consoleInputHandle_CMD, std::placeholders::_1);

void consoleInputHandle_PRIVATE(const std::string& s)
{
    if (s[0] == 'q')
    {
        fsmState = ConsoleState::CMD;
        cb = std::bind(consoleInputHandle_CMD, std::placeholders::_1);
        return;
    }
    
    json body;
    body["type"] = "FriendMessage";
    body["messageChain"] = R"([{"type": "Plain", "text": ""}])"_json;
    body["messageChain"][0]["text"] = s;
    body["sender"] = R"({"id": 12345678, "nickname": "nick", "remark": ""})"_json;

    procRecvMsgEntry(body);
}
void consoleInputHandle_TEMP(const std::string& s)
{
    if (s[0] == 'q')
    {
        fsmState = ConsoleState::CMD;
        cb = std::bind(consoleInputHandle_CMD, std::placeholders::_1);
        return;
    }

    json body;
    body["type"] = "TempMessage";
    body["messageChain"] = R"([{"type": "Plain", "text": ""}])"_json;
    body["messageChain"][0]["text"] = s;
    body["sender"] = R"({"id": 12345678, "memberName": "card", "permission": "MEMBER", "group": {"id": 111111, "name": "group", "permission": "ADMINISTRATOR"}})"_json;
    
    procRecvMsgEntry(body);
}
void consoleInputHandle_GROUP(const std::string& s)
{
    if (s[0] == 'q')
    {
        fsmState = ConsoleState::CMD;
        cb = std::bind(consoleInputHandle_CMD, std::placeholders::_1);
        return;
    }

    json body;
    body["type"] = "GroupMessage";
    body["messageChain"] = R"([{"type": "Plain", "text": ""}])"_json;
    body["messageChain"][0]["text"] = s;
    body["sender"] = R"({"id": 12345678, "memberName": "card", "permission": "MEMBER", "group": {"id": 111111, "name": "group", "permission": "ADMINISTRATOR"}})"_json;
    
    procRecvMsgEntry(body);
}
void consoleInputHandle_CMD(const std::string& s)
{
    switch (s[0])
    {
        case 'q': 
            fsmState = ConsoleState::EXIT; 
            break;
        case '1': 
            fsmState = ConsoleState::PRIVATE;
            cb = std::bind(consoleInputHandle_PRIVATE, std::placeholders::_1);
            break;
        case '2': 
            fsmState = ConsoleState::TEMP;
            cb = std::bind(consoleInputHandle_TEMP, std::placeholders::_1);
            break;
        case '3': 
            fsmState = ConsoleState::GROUP;
            cb = std::bind(consoleInputHandle_GROUP, std::placeholders::_1);
            break;
        default: 
            break;
    }
}

void consoleInputHandle()
{
    std::cout << "--------------------------------------------------------------------------" << std::endl;
    std::cout << "You are running in Debug mode. | Bot qqid: 888888 / Console qqid: 12345678 " << std::endl;
    while (fsmState != ConsoleState::EXIT)
    {
        switch (fsmState)
        {
            case ConsoleState::CMD: 
                std::cout << "Choose Command | q: exit | 1: private msg | 2: temp msg | 3: group msg" << std::endl; 
                break;
            case ConsoleState::PRIVATE: 
                std::cout << "Private Msg: "; 
                break;
            case ConsoleState::TEMP: 
                std::cout << "Temp Msg: "; 
                break;
            case ConsoleState::GROUP: 
                std::cout << "Group Msg: "; 
                break;
            default:
                break;
        }
        std::string buf;
        std::getline(std::cin, buf);
        if (buf.empty()) continue;
        cb(buf);    
    }
}
//procRecvMsgEntry()

#endif

}
