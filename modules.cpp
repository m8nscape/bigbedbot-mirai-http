
#include "core.h"
#include "mirai/api.h"
#include "app/data/user.h"
#include "app/data/group.h"
#include "app/case.h"
#include "app/help.h"
#include "app/tools.h"
#include "app/smoke.h"
#include "app/eatwhat.h"
#include "app/monopoly.h"
#include "app/weather.h"
#include "app/gambol.h"
#include "app/apievent.h"
#include "app/playwhat.h"

#include "time_evt.h"

// #include "app/event_case.h"
// #include "app/gambol.h"
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
    eatwhat::init();
    addTimedEventEveryMin(std::bind(&SQLite::commit, &eatwhat::db, true));
    // eat::updateSteamGameList();
    
    // app: monopoly
    // mnp::calc_event_max();
    monopoly::init("./config/monopoly_chance.yaml");

    // app: weather
    weather::init("./config/weather.yaml");

    // app: playwhat
    playwhat::updateSteamGameList();
    addTimedEvent(playwhat::updateSteamGameList, 0, 0);

    // app: gambol
    gambol::roulette::refreshStock(30000, false);
    addTimedEvent([](){ gambol::roulette::refreshStock(30000, true); }, 12, 0);

    // announce startup
    // std::string boot_info = help::boot_info();
    // broadcastMsg(boot_info.c_str(), grp::MASK_BOOT_ANNOUNCE);

}

void shutdown_modules()
{
    gambol::flipcoin::roundCancelAll();
    gambol::roulette::roundCancelAll();

    eatwhat::db.transactionStop();

}

void add_timed_events()
{
    // 每天刷新群名片
    for (auto it = grp::groups.begin(); it != grp::groups.end(); ++it)
        addTimedEvent(std::bind(&grp::Group::updateMembers, &it->second), 0, 0);

    // 每天刷批
    user::daily_refresh_time = time(nullptr) - 60 * 60 * 24; // yesterday
    user::daily_refresh_tm_auto = *localtime(&user::daily_refresh_time);
    addTimedEvent(std::bind(user::flushDailyTimep, true), user::NEW_DAY_TIME_HOUR, user::NEW_DAY_TIME_MIN);

    /*
    %%%%%%%% steam game list %%%%%%%%
    addTimedEvent([&]() { eat::updateSteamGameList(); }, 0, 0);

    %%%%%%%% 下午6点活动开箱 %%%%%%%%
    addTimedEvent([&]() { event_case::startEvent(); }, event_case::EVENT_CASE_TIME_HOUR_START, event_case::EVENT_CASE_TIME_MIN_START);
    addTimedEvent([&]() { event_case::stopEvent(); }, event_case::EVENT_CASE_TIME_HOUR_END, event_case::EVENT_CASE_TIME_MIN_END);
    */
}

void add_msg_callbacks()
{
    using evt = mirai::RecvMsgType;

    mirai::regEventProc(evt::NewFriendRequestEvent, {"apievent::NewFriendRequestEvent", apievent::NewFriendRequestEvent});

    mirai::regEventProc(evt::GroupMessage, {"opencase::msgDispatcher", opencase::msgDispatcher});

    mirai::regEventProc(evt::FriendMessage, {"help::msgDispatcher", help::msgDispatcher});
    mirai::regEventProc(evt::TempMessage, {"help::msgDispatcher", help::msgDispatcher});
    mirai::regEventProc(evt::GroupMessage, {"help::msgDispatcher", help::msgDispatcher});

    mirai::regEventProc(evt::FriendMessage, {"tools::msgDispatcher", tools::msgDispatcher});
    mirai::regEventProc(evt::TempMessage, {"tools::msgDispatcher", tools::msgDispatcher});
    mirai::regEventProc(evt::GroupMessage, {"tools::msgDispatcher", tools::msgDispatcher});

    mirai::regEventProc(evt::FriendMessage, {"smoke::privateMsgCallback", smoke::privateMsgCallback});
    mirai::regEventProc(evt::TempMessage, {"smoke::privateMsgCallback", smoke::privateMsgCallback});
    mirai::regEventProc(evt::GroupMessage, {"smoke::groupMsgCallback", smoke::groupMsgCallback});
    mirai::regEventProc(evt::MemberMuteEvent, {"smoke::MemberMuteEvent", smoke::MemberMuteEvent});
    mirai::regEventProc(evt::MemberUnmuteEvent, {"smoke::MemberUnmuteEvent", smoke::MemberUnmuteEvent});

    mirai::regEventProc(evt::GroupMessage, {"eatwhat::msgDispatcher", eatwhat::msgDispatcher});

    mirai::regEventProc(evt::GroupMessage, {"monopoly::msgCallback", monopoly::msgCallback});

    mirai::regEventProc(evt::FriendMessage, {"weather::msgCallback", weather::msgCallback});
    mirai::regEventProc(evt::TempMessage, {"weather::msgCallback", weather::msgCallback});
    mirai::regEventProc(evt::GroupMessage, {"weather::msgCallback", weather::msgCallback});

    mirai::regEventProc(evt::GroupMessage, {"gambol::msgDispatcher", gambol::msgDispatcher});
    
    mirai::regEventProc(evt::GroupMessage, {"playwhat::msgDispatcher", playwhat::msgCallback});
}
}