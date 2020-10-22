#include "core.h"
#include "mirai/api.h"
#include "app/case.h"
#include "app/help.h"
#include "app/tools.h"
#include "app/smoke.h"

// #include "app/eat.h"
// #include "app/event_case.h"
// #include "app/gambol.h"
// #include "app/monopoly.h"
// #include "app/user_op.h"
// #include "app/weather.h"

namespace core
{

void init_modules()
{
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

    // announce startup
    //std::string boot_info = help::boot_info();
    //broadcastMsg(boot_info.c_str(), grp::Group::MASK_BOOT_ANNOUNCE);

}

void shutdown_modules()
{
    // for (auto& [group, cfg] : gambol::groupMap)
    // {
    //     if (cfg.flipcoin_running)
    //         gambol::flipcoin::roundCancel(group);
    //     if (cfg.roulette_running)
    // 		gambol::roulette::roundCancel(group);
    // }

    //eat::db.transactionStop();

}

void add_timed_events()
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

void add_msg_callbacks()
{
    using evt = mirai::RecvMsgType;
    mirai::regEventProc(evt::GroupMessage, 	opencase::msgDispatcher);
    mirai::regEventProc(evt::FriendMessage, help::msgDispatcher);
    mirai::regEventProc(evt::TempMessage, 	help::msgDispatcher);
    mirai::regEventProc(evt::GroupMessage, 	help::msgDispatcher);
    mirai::regEventProc(evt::FriendMessage, tools::msgDispatcher);
    mirai::regEventProc(evt::TempMessage, 	tools::msgDispatcher);
    mirai::regEventProc(evt::GroupMessage, 	tools::msgDispatcher);
    mirai::regEventProc(evt::FriendMessage, smoke::privateMsgCallback);
    mirai::regEventProc(evt::TempMessage, 	smoke::privateMsgCallback);
    mirai::regEventProc(evt::GroupMessage, 	smoke::groupMsgCallback);
}

}