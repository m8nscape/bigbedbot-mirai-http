#pragma once
#include <string>
#include <map>
#include <unordered_set>
#include <functional>

#include "../common/dbconn.h"
#include "utils/logger.h"
#include "mirai/api.h"
#include "mirai/util.h"

#include "nlohmann/json.hpp"

namespace grp
{
using nlohmann::json;

inline SQLite db("group.db", "group");
void init();

const int64_t MASK_P = 1 << 0;
const int64_t MASK_EAT = 1 << 1;
const int64_t MASK_GAMBOL = 1 << 2;
const int64_t MASK_MONOPOLY = 1 << 3;
const int64_t MASK_SMOKE = 1 << 4;
const int64_t MASK_CASE = 1 << 5;
const int64_t MASK_EVENT_CASE = 1 << 6;
const int64_t MASK_DAILYP = 1 << 7;
const int64_t MASK_BOOT_ANNOUNCE = 1 << 8;
const int64_t MASK_PLAYWHAT = 1 << 9;
const int64_t MASK_EAT_USE_OLD = 1 << 10;
const int64_t MASK_EAT_USE_UNIVERSE = 1 << 11;

class Group
{
public:
    int64_t group_id = -1;
    int64_t sum_earned = 0;
    int64_t sum_spent = 0;
    int64_t sum_case = 0;
    int64_t sum_card = 0;
    int64_t sum_eatwhat = 0;
    int64_t sum_smoke = 0;

protected:
    int64_t flags = 0;
public:
    std::map<int64_t, mirai::group_member_info> members{};

    void setFlag(int64_t mask, bool set = true);
    bool getFlag(int64_t mask);
    static bool getFlag(int64_t groupid, int64_t mask);

public:
    void updateMembers();
    bool haveMember(int64_t qq) const;
    int64_t getMember(const char* name) const;
    std::string getMemberName(int64_t qq) const;
    void sendMsg(const char* msg) const;
    Group() = default;
    Group(int64_t id) : group_id(id) {}
    void LoadSumFromDb();
    void SaveSumIntoDb();
};

inline std::map<int64_t, Group> groups;

enum class commands : size_t {
    ACTIVATE,
    CONFIGURE,
    SUMMARY
};
typedef std::function<std::string(::int64_t, ::int64_t, std::vector<std::string>&, const char*)> callback;
struct command
{
    commands c = (commands)0;
    std::vector<std::string> args;
    callback func = nullptr;
};

int newGroupIfNotExist(int64_t id);
void msgDispatcher(const json& body);
void MemberJoinEvent(const json& req);
void MemberLeaveEventKick(const json& req);
void MemberLeaveEventQuit(const json& req);
void MemberCardChangeEvent(const json& req);

void broadcastMsg(const char* msg, int64_t flags = -1);

bool checkPermission(int64_t group, int64_t qq, mirai::group_member_permission perm, bool checkRoot);
}
