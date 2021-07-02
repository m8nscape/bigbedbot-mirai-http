#include "pch.h"
#include "util.h"
#include "http_conn.h"
#include "api.h"

int64_t botLoginQQId = 0;
int64_t rootQQId = 0;

namespace mirai
{
    
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