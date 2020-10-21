#pragma once
#include <cstdint>
#include <string>

#include "api.h"

extern int64_t botLoginQQId;
extern int64_t rootQQId;
namespace mirai
{
int getGroupMemberCard(int64_t groupid, int64_t qqid, std::string& card);
int getGroupMemberPermission(int64_t groupid, int64_t qqid, group_member_permission& p);


}