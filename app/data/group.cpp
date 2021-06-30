#include "group.h"

#include <sstream>

#include "utils/logger.h"

#include "mirai/msg.h"
#include "mirai/api.h"

namespace grp
{

void CreateTable()
{
    if (db.exec(
        "CREATE TABLE IF NOT EXISTS grp( \
            id    INTEGER PRIMARY KEY, \
            flags INTEGER         NOT NULL DEFAULT 0 \
         )") != SQLITE_OK)
        addLog(LOG_ERROR, "grp", db.errmsg());
}


void LoadListFromDb()
{
    auto list = db.query("SELECT * FROM grp", 2);
    for (auto& row : list)
    {
        Group g;
        g.group_id = std::any_cast<int64_t>(row[0]);
        
        g.setFlag(std::any_cast<int64_t>(row[1]));

        groups[g.group_id] = g;
    }
    char msg[128];
    sprintf(msg, "added %lu groups", groups.size());
    addLogDebug("grp", msg);
}

void Group::setFlag(int64_t mask, bool set)
{
    if (set)
        flags |= mask;
    else
        flags ^= mask;

    const char query[] = "UPDATE grp SET flags=? WHERE id=?";
    auto ret = db.exec(query, { flags, group_id });
    if (ret != SQLITE_OK)
    {
        char buf[64];
        sprintf(buf, "update flag error: id=%ld, flags=%ld", group_id, flags);
        addLog(LOG_ERROR, "grp", buf);
    }
}

bool Group::getFlag(int64_t mask)
{
    return (mask == -1) || ((flags & mask) == mask);
}

void Group::updateMembers()
{
    char buf[64];
    sprintf(buf, "updating members for group %ld", group_id);
    addLog(LOG_INFO, "grp", buf);

    auto members_tmp = mirai::getGroupMemberList(group_id);
    if (members_tmp.empty())
    {
        char buf[64];
        sprintf(buf, "updating members for group %ld error", group_id);
        addLog(LOG_ERROR, "grp", buf);
        return;
    }

    members.clear();
    for (const auto& m: members_tmp)
        members[m.qqid] = m;

    sprintf(buf, "updated %lu members", members.size());
    addLog(LOG_INFO, "grp", buf);
}

bool Group::haveMember(int64_t qq) const
{
    return members.find(qq) != members.end();
}

int64_t Group::getMember(const char* name) const
{
    for (const auto& [m, v] : members)
    {
        // qqid
        std::string qqid_s = std::to_string(v.qqid);
        if (!strcmp(qqid_s.c_str(), name))
            return m;

        // card
        if (!strcmp(v.nameCard.c_str(), name))
            return m;

        // nickname
        //if (!strcmp(v.nick.c_str(), name))
        //    return m;
    }

    return 0;
}

void Group::sendMsg(const char* msg) const
{
    mirai::sendGroupMsgStr(group_id, std::string(msg));
}

int newGroupIfNotExist(int64_t id)
{
    if (groups.find(id) == groups.end())
    {
        groups[id].updateMembers();

        const char query[] = "INSERT INTO grp(id,flags) VALUES (?,?)";
        auto ret = db.exec(query, { id, 0 });
        if (ret != SQLITE_OK)
        {
            char buf[128];
            sprintf(buf, "insert group error: id=%ld", id);
            addLog(LOG_ERROR, "grp", buf);
        }
    }
    return 0;
}

std::string ENABLE(::int64_t group, ::int64_t qq, std::vector<std::string> args)
{
    if (checkPermission(group, qq, mirai::group_member_permission::ADMINISTRATOR, true))
    {
        auto subcmd = args[0].substr(strlen("开启"));
        auto& g = groups[group];
        if (subcmd == "批")
        {
            g.setFlag(Group::MASK_P);
            return "本群已开启批";
        }
        else if (subcmd == "吃什么")
        {
            g.setFlag(Group::MASK_EAT);
            return "本群已开启吃什么";
        }
        else if (subcmd == "翻批" || subcmd == "摇号")
        {
            g.setFlag(Group::MASK_GAMBOL);
            return "本群已开启翻批/摇号";
        }
        else if (subcmd == "抽卡")
        {
            g.setFlag(Group::MASK_MONOPOLY);
            return "本群已开启抽卡";
        }
        else if (subcmd == "禁烟")
        {
            g.setFlag(Group::MASK_SMOKE);
            return "本群已开启禁烟";
        }
        else if (subcmd == "开箱")
        {
            g.setFlag(Group::MASK_CASE);
            return "本群已开启开箱";
        }
        else if (subcmd == "活动开箱")
        {
            g.setFlag(Group::MASK_EVENT_CASE);
            return "本群已开启活动开箱";
        }
        else if (subcmd == "每日批池")
        {
            g.setFlag(Group::MASK_DAILYP);
            return "本群已开启每日批池";
        }
        else if (subcmd == "启动信息")
        {
            g.setFlag(Group::MASK_BOOT_ANNOUNCE);
            return "本群已开启启动信息";
        }
    }
    //return "你开个锤子？";
    return "";
}

std::string DISABLE(::int64_t group, ::int64_t qq, std::vector<std::string> args)
{
    if (checkPermission(group, qq, mirai::group_member_permission::ADMINISTRATOR, true))
    {
        auto subcmd = args[0].substr(strlen("关闭"));
        auto& g = groups[group];
        if (subcmd == "批")
        {
            g.setFlag(Group::MASK_EAT, false);
            return "本群已关闭批";
        }
        else if (subcmd == "吃什么")
        {
            g.setFlag(Group::MASK_EAT, false);
            return "本群已关闭吃什么";
        }
        else if (subcmd == "翻批" || subcmd == "摇号")
        {
            g.setFlag(Group::MASK_GAMBOL, false);
            return "本群已关闭翻批/摇号";
        }
        else if (subcmd == "抽卡")
        {
            g.setFlag(Group::MASK_MONOPOLY, false);
            return "本群已关闭抽卡";
        }
        else if (subcmd == "禁烟")
        {
            g.setFlag(Group::MASK_SMOKE, false);
            return "本群已关闭禁烟";
        }
        else if (subcmd == "开箱")
        {
            g.setFlag(Group::MASK_CASE, false);
            return "本群已关闭开箱";
        }
        else if (subcmd == "活动开箱")
        {
            g.setFlag(Group::MASK_EVENT_CASE, false);
            return "本群已关闭活动开箱";
        }
        else if (subcmd == "每日批池")
        {
            g.setFlag(Group::MASK_DAILYP);
            return "本群已关闭每日批池";
        }
        else if (subcmd == "启动信息")
        {
            g.setFlag(Group::MASK_BOOT_ANNOUNCE);
            return "本群已关闭启动信息";
        }
    }
    //return "你关个锤子？";
    return "";
}

std::string QUERY_FLAGS(::int64_t group, ::int64_t qq, std::vector<std::string> args)
{
    if (checkPermission(group, qq, mirai::group_member_permission::ADMINISTRATOR, true))
    {
        auto& g = groups[group];
        std::stringstream ss;
        ss << "批: " << (g.getFlag(Group::MASK_P) ? "Y" : "N") << std::endl;
        ss << "吃什么: " << (g.getFlag(Group::MASK_EAT) ? "Y" : "N") << std::endl;
        //ss << "翻批: " << (g.getFlag(Group::MASK_GAMBOL) ? "Y" : "N") << std::endl;
        ss << "抽卡: " << (g.getFlag(Group::MASK_MONOPOLY) ? "Y" : "N") << std::endl;
        ss << "禁烟: " << (g.getFlag(Group::MASK_SMOKE) ? "Y" : "N") << std::endl;
        ss << "开箱: " << (g.getFlag(Group::MASK_CASE) ? "Y" : "N") << std::endl;
        //ss << "活动开箱: " << (g.getFlag(Group::MASK_EVENT_CASE) ? "Y" : "N") << std::endl;
        ss << "领批: " << (g.getFlag(Group::MASK_DAILYP) ? "Y" : "N") << std::endl;
        ss << "启动公告: " << (g.getFlag(Group::MASK_BOOT_ANNOUNCE) ? "Y" : "N");
        return ss.str();
    }
    return "就你也想看权限？";
}

void msgDispatcher(const json& body)
{
    auto query = mirai::messageChainToArgs(body);
    if (query.empty()) return;

    auto m = mirai::parseMsgMetadata(body);

    if (m.groupid == 0)
    {
        return;
    }

    if (groups.find(m.groupid) == groups.end())
    {
        newGroupIfNotExist(m.groupid);
    }
    
    auto cmd = query[0];
    std::string resp;
    if (cmd.substr(0, sizeof("开启")-1) == "开启")
    {
        resp = ENABLE(m.groupid, m.qqid, query);
    }
    else if (cmd.substr(0, sizeof("关闭")-1) == "关闭")
    {
        resp = DISABLE(m.groupid, m.qqid, query);
    }
    else if (cmd.substr(0, sizeof("权限")-1) == "权限")
    {
        resp = QUERY_FLAGS(m.groupid, m.qqid, query);
    }

    if (!resp.empty())
    {
        mirai::sendGroupMsgStr(m.groupid, resp);
    }
}

void broadcastMsg(const char* msg, int64_t flag)
{
    for (auto& [id, g] : grp::groups)
    {
        //CQ_sendGroupMsg(ac, group, msg);
        if (!g.members.empty() && g.getFlag(flag))
            g.sendMsg(msg);
    }
}

bool checkPermission(int64_t group, int64_t qq, mirai::group_member_permission permRequired, bool checkRoot = false)
{
    // grp::groups[group].members[qq].permission
    using p = mirai::group_member_permission;
    if (checkRoot && qq == rootQQId) 
        return p::ROOT >= permRequired;

    auto qqPerm = p::NO_GROUP;
    if (groups.find(group) == groups.end()) qqPerm = p::NO_GROUP;
    else if (!groups.at(group).haveMember(qq)) qqPerm = p::NO_MEMBER;
    else qqPerm = groups[group].members[qq].permission;

    //addLog(LOG_DEBUG, "group", "QQ:%lld Permission:%d Required:%d", qq, (int)qqPerm, (int)permRequired);

    return qqPerm >= permRequired;
}

void init()
{
    CreateTable();
    LoadListFromDb();
    for (auto& [groupid, groupObj] : grp::groups)
        groupObj.updateMembers();
}

}
