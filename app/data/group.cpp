#include "group.h"

#include <sstream>

#include "utils/logger.h"
#include "utils/rand.h"
#include "time_evt.h"

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

    if (db.exec(
        "CREATE TABLE IF NOT EXISTS grpsum( \
            id      INTEGER PRIMARY KEY, \
            sum_earned  INTEGER         NOT NULL DEFAULT 0, \
            sum_spent   INTEGER         NOT NULL DEFAULT 0, \
            sum_case    INTEGER         NOT NULL DEFAULT 0, \
            sum_card    INTEGER         NOT NULL DEFAULT 0, \
            sum_eatwhat INTEGER         NOT NULL DEFAULT 0, \
            sum_smoke   INTEGER         NOT NULL DEFAULT 0 \
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
    addLogDebug("grp", "added %lu groups", groups.size());

    // update for new table
    for (const auto& g: groups)
    {
        int64_t id = g.first;
        const char query1[] = "INSERT OR IGNORE INTO grpsum(id) VALUES (?)";
        auto ret = db.exec(query1, { id });
        if (ret != SQLITE_OK)
        {
            addLog(LOG_ERROR, "grp", "insert group summary error: id=%ld", id);
        }
    }

}

void Group::LoadSumFromDb()
{
    auto list = db.query("SELECT * FROM grpsum WHERE id = ?", 7, {group_id});
    for (auto& row : list)
    {
        auto id = std::any_cast<int64_t>(row[0]);
        if (id == group_id)
        {
            sum_earned = std::any_cast<int64_t>(row[1]);
            sum_spent = std::any_cast<int64_t>(row[2]);
            sum_case = std::any_cast<int64_t>(row[3]);
            sum_card = std::any_cast<int64_t>(row[4]);
            sum_eatwhat = std::any_cast<int64_t>(row[5]);
            sum_smoke = std::any_cast<int64_t>(row[6]);
            break;
        }
    }
}
void Group::SaveSumIntoDb()
{
    auto ret = db.exec("UPDATE grpsum SET \
sum_earned=?, \
sum_spent=?, \
sum_case=?, \
sum_card=?, \
sum_eatwhat=?, \
sum_smoke=? \
WHERE id=?", {
    sum_earned,
    sum_spent,
    sum_case,
    sum_card,
    sum_eatwhat,
    sum_smoke,
    group_id
});
    if (ret != SQLITE_OK)
    {
        addLog(LOG_ERROR, "grp", "update summary error: id=%ld", group_id);
    }
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
        addLog(LOG_ERROR, "grp", "update flag error: id=%ld, flags=%ld", group_id, flags);
    }
}

bool Group::getFlag(int64_t mask)
{
    return (mask == -1) || ((flags & mask) == mask);
}

bool Group::getFlag(int64_t groupid, int64_t mask)
{
    if (groups.find(groupid) != groups.end())
        return groups[groupid].getFlag(mask);
    else
        return false;
}

void Group::updateMembers()
{
    addLog(LOG_INFO, "grp", "updating members for group %ld", group_id);

    auto members_tmp = mirai::getGroupMemberList(group_id);
    if (members_tmp.empty())
    {
        addLog(LOG_ERROR, "grp", "updating members for group %ld error", group_id);
        return;
    }

    members.clear();
    for (const auto& m: members_tmp)
        members[m.qqid] = m;

    addLog(LOG_INFO, "grp", "updated %lu members", members.size());
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

std::string Group::getMemberName(int64_t qq) const
{
    return haveMember(qq) ? members.at(qq).nameCard : "";
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
            addLog(LOG_ERROR, "grp", "insert group error: id=%ld", id);
        }
        
        const char query1[] = "INSERT INTO grpsum(id) VALUES (?)";
        ret = db.exec(query1, { id });
        if (ret != SQLITE_OK)
        {
            addLog(LOG_ERROR, "grp", "insert group summary error: id=%ld", id);
        }
        
        addTimedEventEveryMin(std::bind(&grp::Group::SaveSumIntoDb, groups[id]));
    }
    return 0;
}

const std::map<int64_t, std::pair<std::set<std::string>, std::string>> flagMap = 
{
    std::make_pair(MASK_P, 
        std::make_pair(std::set<std::string>{"批"}, "批")),
    std::make_pair(MASK_EAT, 
        std::make_pair(std::set<std::string>{"吃什么"}, "吃什么")),
    std::make_pair(MASK_GAMBOL, 
        std::make_pair(std::set<std::string>{"翻批", "摇号"}, "翻批/摇号")),
    std::make_pair(MASK_MONOPOLY, 
        std::make_pair(std::set<std::string>{"抽卡"}, "抽卡")),
    std::make_pair(MASK_SMOKE, 
        std::make_pair(std::set<std::string>{"禁烟"}, "禁烟")),
    std::make_pair(MASK_CASE, 
        std::make_pair(std::set<std::string>{"开箱"}, "开箱")),
    std::make_pair(MASK_EVENT_CASE, 
        std::make_pair(std::set<std::string>{"活动开箱"}, "活动开箱")),
    std::make_pair(MASK_DAILYP, 
        std::make_pair(std::set<std::string>{"每日批池", "领批"}, "每日批池")),
    std::make_pair(MASK_BOOT_ANNOUNCE, 
        std::make_pair(std::set<std::string>{"启动信息"}, "启动信息")),
    std::make_pair(MASK_PLAYWHAT, 
        std::make_pair(std::set<std::string>{"玩什么"}, "玩什么")),
    std::make_pair(MASK_EAT_USE_OLD, 
        std::make_pair(std::set<std::string>{"吃什么旧菜单"}, "吃什么旧菜单")),
    std::make_pair(MASK_EAT_USE_UNIVERSE, 
        std::make_pair(std::set<std::string>{"吃什么宇宙"}, "吃什么能返回所有群加的菜")),
};

std::string ENABLE(::int64_t group, ::int64_t qq, std::vector<std::string> args)
{
    if (checkPermission(group, qq, mirai::group_member_permission::ADMINISTRATOR, true))
    {
        auto subcmd = args[0].substr(strlen("开启"));
        auto& g = groups[group];
        for (const auto& [mask, info]: flagMap)
        {
            const auto& [keywords, msg] = info;
            if (keywords.find(subcmd) == keywords.end()) continue;

            if (mask == MASK_GAMBOL)
            {
                g.setFlag(mask, false);
                using namespace std::string_literals;
                return "本群已关闭"s + msg;
            }

            g.setFlag(mask, true);
            using namespace std::string_literals;
            return "本群已开启"s + msg;
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
        for (const auto& [mask, info]: flagMap)
        {
            const auto& [keywords, msg] = info;
            if (keywords.find(subcmd) == keywords.end()) continue;

            g.setFlag(mask, false);
            using namespace std::string_literals;
            return "本群已关闭"s + msg;
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
        bool isFirst = true;
        for (const auto& [mask, info]: flagMap)
        {
            if (mask == grp::MASK_GAMBOL) continue;
            if (mask == grp::MASK_EVENT_CASE) continue;
            const auto& [keywords, msg] = info;
            if (!isFirst) ss << std::endl;
            isFirst = false;
            ss << *keywords.begin() << ": " << (g.getFlag(mask) ? "Y" : "N");
        }
        return ss.str();
    }
    return "就你也想看权限？";
}

std::string SUMMARY(int64_t group, ::int64_t qq, std::vector<std::string>& args)
{
    auto& g = groups[group];
    std::stringstream ss;
    ss << "本群一共" << std::endl;
    if (g.getFlag(MASK_P))
    {
        ss << "赚到 " << g.sum_earned << " 个批" << std::endl;
        ss << "花出 " << g.sum_spent << " 个批" << std::endl;
        if (g.getFlag(MASK_CASE))
            ss << "开了 " << g.sum_case << " 个箱" << std::endl;
        if (g.getFlag(MASK_MONOPOLY))
            ss << "抽了 " << g.sum_card << " 张卡" << std::endl;
    }
    if (g.getFlag(MASK_EAT))
        ss << "吃了 " << g.sum_eatwhat << " 次什么" << std::endl;
    if (g.getFlag(MASK_SMOKE))
        ss << "禁烟群友 " << g.sum_smoke << " 分钟" << std::endl;
    switch (randInt(0, 3))
    {
        case 0: ss << "各位水友继续努力/cy"; break;
        case 1: ss << "上班多水群，少上班"; break;
        case 2: ss << "群友们平时一定很无聊8"; break;
        case 3: ss << "批到底有什么用啊"; break;
        default: break;
    }
    return ss.str();
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
    else if (cmd.substr(0, sizeof("统计")-1) == "统计")
    {
        resp = SUMMARY(m.groupid, m.qqid, query);
    }

    if (!resp.empty())
    {
        mirai::sendGroupMsgStr(m.groupid, resp);
    }
}

void MemberJoinEvent(const json& req)
{
    auto [groupid, qqid] = mirai::parseIdFromGroupEvent(req);
    if (groupid != 0 && qqid != 0)
    {
        const auto& m = req.at("member");
        mirai::group_member_info g;
        if (m.contains("id"))
            g.qqid = m.at("id");
        if (m.contains("memberName"))
            g.nameCard = m.at("memberName");
        if (m.contains("permission"))
        {
            const auto& p = m.at("permission");
            if (p == "ADMINISTRATOR") 
                g.permission = mirai::group_member_permission::ADMINISTRATOR;
            else if (p == "OWNER") 
                g.permission = mirai::group_member_permission::OWNER;
            else 
                g.permission = mirai::group_member_permission::MEMBER;
        }

        grp::groups[groupid].members[g.qqid] = g;

        addLog(LOG_INFO, "grp", "added member %lu to group %lu", g.qqid, groupid);
    }
}

void MemberLeaveEventKick(const json& req)
{
    auto [groupid, qqid] = mirai::parseIdFromGroupEvent(req);
    if (groupid != 0 && qqid != 0)
    {
        const auto& m = req.at("member");
        auto& g = grp::groups[groupid].members;
        if (g.find(qqid) != g.end())
        {
            g.erase(qqid);
            addLog(LOG_INFO, "grp", "removed member %lu from group %lu", qqid, groupid);
        }
    }
}

void MemberLeaveEventQuit(const json& req)
{
    MemberLeaveEventKick(req);
}

void MemberCardChangeEvent(const json& req)
{
    auto [groupid, qqid] = mirai::parseIdFromGroupEvent(req);
    if (groupid != 0 && qqid != 0)
    {
        const auto& m = req.at("member");
        if (req.contains("current"))
        {
            std::string current = req.at("current");
            auto& g = grp::groups[groupid].members;
            if (g.find(qqid) != g.end())
            {
                g[qqid].nameCard = current;
                addLog(LOG_INFO, "grp", "modified member card %lu in group %lu to %s", qqid, groupid, current.c_str());
            }
        }

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
    {
        groupObj.updateMembers();
        groupObj.LoadSumFromDb();
    }
}

}
