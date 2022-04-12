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
#include "app/api/api.h"

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
        addLog(LOG_ERROR, "core", "Config file %s not found", fs::absolute(cfgPath).c_str());
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
    gLogLevel = cfg["log_level"].as<int>();
    bbb::api::inst.set_port(cfg["api_port"].as<short>());
#else
    botLoginQQId = 888888;
    rootQQId = 12345678;
    useWebsocket = false;
    gLogLevel = LOG_DEBUG;
    bbb::api::inst.set_port(15555);
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
    mirai::regEventProc(mirai::RecvMsgType::GroupMessage, {"UserMessage", user::msgCallback});

    // app: group
    grp::init();
    addTimedEventEveryMin(std::bind(&SQLite::commit, &grp::db, true));
    mirai::regEventProc(mirai::RecvMsgType::GroupMessage, {"GroupMessage", grp::msgDispatcher});
    mirai::regEventProc(mirai::RecvMsgType::MemberJoinEvent, {"grp::MemberJoinEvent", grp::MemberJoinEvent});
    mirai::regEventProc(mirai::RecvMsgType::MemberLeaveEventKick, {"grp::MemberLeaveEventKick", grp::MemberLeaveEventKick});
    mirai::regEventProc(mirai::RecvMsgType::MemberLeaveEventQuit, {"grp::MemberLeaveEventQuit", grp::MemberLeaveEventQuit});
    mirai::regEventProc(mirai::RecvMsgType::MemberCardChangeEvent, {"grp::MemberCardChangeEvent", grp::MemberCardChangeEvent});
    // 每分钟保存群统计
    for (auto it = grp::groups.begin(); it != grp::groups.end(); ++it)
        addTimedEventEveryMin(std::bind(&grp::Group::SaveSumIntoDb, &it->second));

    // app: api
    bbb::api::inst.start();

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

        bbb::api::inst.stop();

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
}