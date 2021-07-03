#include <thread>

#include <curl/curl.h>
#include <yaml-cpp/yaml.h>

#if __GNUC__ >= 8
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

#include "core.h"

#include "app/data/user.h"
#include "app/data/group.h"

#include "mirai/http_conn.h"
#include "mirai/ws_conn.h"
#include "mirai/api.h"
#include "mirai/util.h"

#include "time_evt.h"
#include "utils/logger.h"

namespace core
{

bool botStarted = false;
bool useWebsocket = true;

bool isBotStarted() {return botStarted;}

int initialize()
{
    addLog(LOG_INFO, "core", "Initializing...");

    if (config() != 0)
    {
        addLog(LOG_ERROR, "core", "Load config failed!");
        return -255;
    }

    if (mirai::registerApp() < 0)
    {
        addLog(LOG_ERROR, "core", "Authenticating with mirai core failed. It is possibly that core is not running correctly, or Auth Key is incorrent");
        return -254;
    }

    curl_global_init(CURL_GLOBAL_ALL);

    addLog(LOG_INFO, "core", "Initialization succeeded.");
    
    return 0;
}

int config()
{
    const char* cfgFile = "./config/config.yaml";
    fs::path cfgPath(cfgFile);
    if (!fs::is_regular_file(cfgPath))
    {
        addLog(LOG_ERROR, "core", "Config file %s not found", fs::absolute(cfgPath));
        return -1;
    }
    addLog(LOG_INFO, "core", "Loading config from %s", fs::absolute(cfgPath).c_str());
    YAML::Node cfg = YAML::LoadFile(cfgFile);
    mirai::setAuthKey(cfg["authkey"].as<std::string>());
    unsigned short port = cfg["port"].as<unsigned short>();
    mirai::http::set_port(port);
    mirai::ws::set_port(port);
#ifdef NDEBUG
    botLoginQQId = cfg["qq"].as<int64_t>();
    rootQQId = cfg["qq_root"].as<int64_t>();
    useWebsocket = cfg["use_ws"].as<bool>();
#else
    botLoginQQId = 888888;
    rootQQId = 12345678;
    useWebsocket = false;
#endif
    return 0;
}

int init_app_and_start()
{
    if (botStarted) return -1;
    botStarted = true;

    addLog(LOG_INFO, "core", "Starting");

    // app: user
    user::init("./config/user.yaml");
    addTimedEventEveryMin(std::bind(&SQLite::commit, &user::db, true));
    mirai::regEventProc(mirai::RecvMsgType::GroupMessage, grp::msgDispatcher);

    // app: group
    grp::init();
    addTimedEventEveryMin(std::bind(&SQLite::commit, &grp::db, true));
    mirai::regEventProc(mirai::RecvMsgType::GroupMessage, user::msgCallback);
    // 每分钟保存群统计
    for (auto it = grp::groups.begin(); it != grp::groups.end(); ++it)
        addTimedEventEveryMin(std::bind(&grp::Group::SaveSumIntoDb, &it->second));
        

    init_modules();
    add_timed_events();
    add_msg_callbacks();

    // start threads
    startTimedEvent();

#ifdef NDEBUG
    // listen events
    if (useWebsocket)
        mirai::connectMsgWebSocket();
    else
        mirai::startMsgPoll();
#else
    std::thread([](){mirai::consoleInputHandle(); core::shutdown();}).detach();
#endif
    
    return 0;
}


int shutdown()
{
    if (botStarted)
    {
        addLog(LOG_INFO, "core", "Stopping");

        for (auto it = grp::groups.begin(); it != grp::groups.end(); ++it)
            it->second.SaveSumIntoDb();

        shutdown_modules();
        user::db.transactionStop();
        grp::db.transactionStop();
        botStarted = false;
#ifdef NDEBUG
        if (useWebsocket) 
            mirai::disconnectMsgWebSocket();
        else
            mirai::stopMsgPoll();
#endif
        stopTimedEvent();
    }

    curl_global_cleanup();
    return 0;
}


/*
* Type=21 私聊消息
* subType 子类型，11/来自好友 1/来自在线状态 2/来自群 3/来自讨论组
*/
/*
int onPrivateMsg(int32_t msgId, int64_t fromQQ, const char *msg, int32_t font) 
{
    if (!strcmp(msg, "接近我") || !strcmp(msg, "解禁我"))
    {
        CQ_sendPrivateMsg(ac, fromQQ, smoke::selfUnsmoke(fromQQ).c_str());
        return EVENT_BLOCK;
    }

    //如果要回复消息，请调用酷Q方法发送，并且这里 return EVENT_BLOCK - 截断本条消息，不再继续处理  注意：应用优先级设置为"最高"(10000)时，不得使用本返回值
    //如果不回复消息，交由之后的应用/过滤器处理，这里 return EVENT_IGNORE - 忽略本条消息
    return EVENT_IGNORE;
}
*/

//time_t banTime_me = 0;

/*
CQEVENT(int32_t, __unsmokeAll, 0)() 
{
    time_t t = time(nullptr);
    for (auto& c : smoke::smokeTimeInGroups)
        for (auto& g : c.second)
            if (t < g.second)
                mirai::unmute(g.first, c.first);
    return 0;
}


CQEVENT(int32_t, __updateSteamGameList, 0)() 
{
    eat::updateSteamGameList();
    return 0;
}
*/
}