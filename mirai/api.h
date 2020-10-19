#pragma once
#include <string>
#include <functional>
#include <vector>

#include <nlohmann/json.hpp>

namespace mirai
{
int auth();
int verify();

using nlohmann::json;

// SEND
int sendPrivateMsg(int64_t qqid, int64_t groupid, const std::string& msg, int64_t quotemsgid = 0);
int sendPrivateMsg(int64_t qqid, int64_t groupid, const json& messageChain, int64_t quotemsgid = 0);
int sendGroupMsg(int64_t groupid, const std::string& msg, int64_t quotemsgid = 0);
int sendGroupMsg(int64_t groupid, const json& messageChain, int64_t quotemsgid = 0);
int recallMsg(int64_t msgid);
int mute(int64_t qqid, int64_t groupid, int time_sec);
inline int unmute(int64_t qqid, int64_t groupid, int time_sec) { return mute(qqid, groupid, 0); }

// RECV
using nlohmann::json;

typedef std::function<void(const json&)> MessageProc;

enum class RecvMsgType
{
    GroupMessage,
    FriendMessage,
    TempMessage,
    BotOnlineEvent,
    BotOfflineEventActive,
    BotOfflineEventForce,
    BotOfflineEventDropped,
    BotReloginEvent,
    GroupRecallEvent,
    FriendRecallEvent,
    BotGroupPermissionChangeEvent,
    BotMuteEvent,
    BotUnmuteEvent,
    BotJoinGroupEvent,
    BotLeaveEventActive,
    BotLeaveEventKick,
    GroupNameChangeEvent,
    GroupEntranceAnnouncementChangeEvent,
    GroupMuteAllEvent,
    GroupAllowAnonymousChatEvent,
    GroupAllowConfessTalkEvent,
    GroupAllowMemberInviteEvent,
    MemberJoinEvent,
    MemberLeaveEventKick,
    MemberLeaveEventQuit,
    MemberCardChangeEvent,
    MemberSpecialTitleChangeEvent,
    MemberPermissionChangeEvent,
    MemberMuteEvent,
    MemberUnmuteEvent,
    NewFriendRequestEvent,
    MemberJoinRequestEvent,
    BotInvitedJoinGroupRequestEvent,
};
int regEventProc(RecvMsgType evt, MessageProc cb);


enum class group_member_permission
{
    MEMBER = 0,
    ADMINISTRATOR = 1,
    OWNER = 2,
};
struct group_member_info
{
    int64_t qqid = 0;
    group_member_permission permission = group_member_permission::MEMBER;
    int muteTimestamp = 0;
    std::string nameCard;
    std::string specialTitle;
};
int getGroupMemberInfo(int64_t groupid, int64_t qqid, group_member_info& g);
std::vector<group_member_info> getGroupMemberList(int64_t groupid);

const int POLLING_INTERVAL_MS = 1000;
const int POLLING_QUANTITY = 10;

void startMsgPoll();
void stopMsgPoll();

}
