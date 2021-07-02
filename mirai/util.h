#pragma once
#include <cstdint>
#include <string>

extern int64_t botLoginQQId;
extern int64_t rootQQId;
namespace mirai
{
std::string getGroupMemberCard(int64_t groupid, int64_t qqid);
//group_member_permission getGroupMemberPermission(int64_t groupid, int64_t qqid);

}