#include <thread>

#include <curl/curl.h>
#include <yaml-cpp/yaml.h>
#include <filesystem>

#include "core.h"

#include "app/data/user.h"
#include "app/data/group.h"

#include "app/case.h"
#include "app/help.h"
#include "app/tools.h"

// #include "app/eat.h"
// #include "app/event_case.h"
// #include "app/gambol.h"
// #include "app/monopoly.h"
// #include "app/smoke.h"
// #include "app/user_op.h"
// #include "app/weather.h"


#include "mirai/http_conn.h"
#include "mirai/api.h"
#include "mirai/util.h"

#include "time_evt.h"
#include "logger.h"

namespace core
{

std::string authKey;
bool botStarted = false;
bool isBotStarted() {return botStarted;}

int initialize()
{
	addLog(LOG_INFO, "core", "Initializing...");

	if (config() != 0)
	{
		addLog(LOG_ERROR, "core", "Load config failed!");
		return -255;
	}

	if (mirai::auth(authKey.c_str()) != 0)
	{
		addLog(LOG_ERROR, "core", "Auth failed!");
		return -1;
	}

	if (mirai::verify() != 0)
	{
		addLog(LOG_ERROR, "core", "Verify failed!");
		return -2;
	}

	curl_global_init(CURL_GLOBAL_ALL);

	addLog(LOG_INFO, "core", "Initialization succeeded.");
	
	return 0;
}

int config()
{
	const char* cfgFile = "./config/config.yaml";
	std::filesystem::path cfgPath(cfgFile);
	if (!std::filesystem::is_regular_file(cfgPath))
	{
		addLog(LOG_ERROR, "core", "Config file %s not found", std::filesystem::absolute(cfgPath));
		return -1;
	}
	addLog(LOG_INFO, "core", "Loading config from %s", std::filesystem::absolute(cfgPath).c_str());
	YAML::Node cfg = YAML::LoadFile(cfgPath);
	authKey = cfg["authkey"].as<std::string>();
	mirai::conn::set_port(cfg["port"].as<unsigned short>());
	botLoginQQId = cfg["qq"].as<int64_t>();
	rootQQId = cfg["qq_root"].as<int64_t>();
	return 0;
}

void addTimedEvents();
void addMsgCallbacks();

int init_app_and_start()
{
	if (botStarted) return -1;

	addLog(LOG_INFO, "core", "Starting");

	botStarted = true;

	// app: user
    user::init("./config/user.yaml");
	addTimedEventEveryMin(std::bind(&SQLite::commit, &user::db, true));

	// app: group
	grp::init();
	addTimedEventEveryMin(std::bind(&SQLite::commit, &grp::db, true));

	// app: case
	opencase::init("./config/case.yaml");

	// app: event_case
	// event_case::init("./app/event_case_draw.yaml", "./app/eent_case_drop.yaml");

	// app: eat
    // eat::foodCreateTable();
	// eat::drinkCreateTable();
    // //eat::foodLoadListFromDb();
    // eat::updateSteamGameList();
	
	// app: monopoly
    //mnp::calc_event_max();

	addTimedEvents();
	addMsgCallbacks();

	// announce startup
    //std::string boot_info = help::boot_info();
    //broadcastMsg(boot_info.c_str(), grp::Group::MASK_BOOT_ANNOUNCE);

	// start threads
	startTimedEvent();
	mirai::startMsgPoll();
	
	return 0;
}


int shutdown()
{
    if (botStarted)
    {
        // for (auto& [group, cfg] : gambol::groupMap)
        // {
        //     if (cfg.flipcoin_running)
        //         gambol::flipcoin::roundCancel(group);
        //     if (cfg.roulette_running)
		// 		gambol::roulette::roundCancel(group);
        // }
        user::db.transactionStop();
		grp::db.transactionStop();
		//eat::db.transactionStop();
        botStarted = false;
		mirai::stopMsgPoll();
		stopTimedEvent();
    }

    curl_global_cleanup();
	return 0;
}

void addTimedEvents()
{
	/*
	// 每天刷新群名片
	for (auto&[group, groupObj] : grp::groups)
		addTimedEvent([&]() { groupObj.updateMembers(); }, 0, 0);

	// steam game list
	addTimedEvent([&]() { eat::updateSteamGameList(); }, 0, 0);

	// 每天刷批
	user_op::daily_refresh_time = time(nullptr) - 60 * 60 * 24; // yesterday
	user_op::daily_refresh_tm_auto = getLocalTime(TIMEZONE_HR, TIMEZONE_MIN);
	addTimedEvent([&]() { user_op::flushDailyTimep(true); }, user_op::NEW_DAY_TIME_HOUR, user_op::NEW_DAY_TIME_MIN);

    // 照顾美国人
	addTimedEvent([&]() { event_case::startEvent(); }, 4, 0);
	addTimedEvent([&]() { event_case::stopEvent(); }, 5, 0);

	// 下午6点活动开箱
	addTimedEvent([&]() { event_case::startEvent(); }, event_case::EVENT_CASE_TIME_HOUR_START, event_case::EVENT_CASE_TIME_MIN_START);
	addTimedEvent([&]() { event_case::stopEvent(); }, event_case::EVENT_CASE_TIME_HOUR_END, event_case::EVENT_CASE_TIME_MIN_END);
	*/

}

void addMsgCallbacks()
{
	using evt = mirai::RecvMsgType;
	mirai::regEventProc(evt::GroupMessage, 	grp::msgDispatcher);
	mirai::regEventProc(evt::GroupMessage, 	user::msgCallback);
	mirai::regEventProc(evt::GroupMessage, 	opencase::msgDispatcher);
	mirai::regEventProc(evt::FriendMessage, help::msgDispatcher);
	mirai::regEventProc(evt::TempMessage, 	help::msgDispatcher);
	mirai::regEventProc(evt::GroupMessage, 	help::msgDispatcher);
	mirai::regEventProc(evt::FriendMessage, tools::msgDispatcher);
	mirai::regEventProc(evt::TempMessage, 	tools::msgDispatcher);
	mirai::regEventProc(evt::GroupMessage, 	tools::msgDispatcher);
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