#include <sstream>
#include <future>

#include "nlohmann/json.hpp"

#include "util.h"
#include "http_conn.h"


int64_t botLoginQQId = 0;
int64_t rootQQId = 0;

namespace mirai
{
using nlohmann::json;

std::string getGroupMemberCard(int64_t groupid, int64_t qqid)
{
    group_member_info g = getGroupMemberInfo(groupid, qqid);
    if (g.qqid == qqid)
    {
        return g.nameCard;
    }
    return "";
}

}