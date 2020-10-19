#include <sstream>
#include <future>

#include "nlohmann/json.hpp"

#include "util.h"
#include "http_conn.h"


extern char miraiSession[64];
bool gBotEnabled = false;
int64_t QQME;

namespace mirai
{
using nlohmann::json;

int getGroupMemberCard(int64_t groupid, int64_t qqid, std::string& card)
{
    group_member_info g;
    int ret = getGroupMemberInfo(groupid, qqid, g);
    if (ret == 0)
    {
        card = g.nameCard;
    }
    return ret;
}

int getGroupMemberPermission(int64_t groupid, int64_t qqid, group_member_permission& p)
{
    group_member_info g;
    int ret = getGroupMemberInfo(groupid, qqid, g);
    if (ret == 0)
    {
        p = g.permission;
    }
    return ret;
}

}