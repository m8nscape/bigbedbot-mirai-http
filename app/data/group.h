#pragma once
#include <string>
#include <map>
#include <unordered_set>
#include <functional>

#include "../common/dbconn.h"
#include "utils/logger.h"
#include "mirai/util.h"

#include "nlohmann/json.hpp"

namespace grp
{
using nlohmann::json;

inline SQLite db("group.db", "group");
void init();
class Group
{
public:
    int64_t group_id = -1;
protected:
	int64_t flags = 0;
public:
    std::map<int64_t, mirai::group_member_info> members{};

	static const int64_t MASK_P = 1 << 0;
	static const int64_t MASK_EAT = 1 << 1;
	static const int64_t MASK_GAMBOL = 1 << 2;
	static const int64_t MASK_MONOPOLY = 1 << 3;
	static const int64_t MASK_SMOKE = 1 << 4;
	static const int64_t MASK_CASE = 1 << 5;
	static const int64_t MASK_EVENT_CASE = 1 << 6;
	static const int64_t MASK_DAILYP = 1 << 7;
	static const int64_t MASK_BOOT_ANNOUNCE = 1 << 8;
	void setFlag(int64_t mask, bool set = true);
	bool getFlag(int64_t mask);

public:
    void updateMembers();
    bool haveMember(int64_t qq) const;
    int64_t getMember(const char* name) const;
    void sendMsg(const char* msg) const;
    Group() = default;
    Group(int64_t id) : group_id(id) {}
};

inline std::map<int64_t, Group> groups;

enum class commands : size_t {
	激活,
	设置
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

void broadcastMsg(const char* msg, int64_t flags = -1);
}
