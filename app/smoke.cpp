#include <sstream>
#include <utility>
#include <regex>

#include "smoke.h"
#include "utils/rand.h"

#include "data/user.h"
#include "data/group.h"

#include "mirai/api.h"
#include "mirai/msg.h"
#include "mirai/util.h"
namespace smoke
{

RetVal nosmoking(int64_t group, int64_t target, int duration)
{
    if (duration < 0) return RetVal::INVALID_DURATION;

    if (grp::groups.find(group) != grp::groups.end())
    {
        // get group info from cache
        if (grp::groups[group].haveMember(target))
        {
            auto perm = grp::groups[group].members[target].permission;
            if (perm == mirai::group_member_permission::ADMINISTRATOR
                || perm == mirai::group_member_permission::OWNER)
                return RetVal::TARGET_IS_ADMIN;
        }
        else
            return RetVal::TARGET_NOT_FOUND;
    }
    else
    {
        return RetVal::TARGET_NOT_FOUND;
    }

    mirai::mute(target, group, duration);
    
    if (duration == 0)
    {
        smokeTimeInGroups[target].erase(group);
        return RetVal::ZERO_DURATION;
    }

    smokeTimeInGroups[target][group] = time(nullptr) + int64_t(duration) * 60;
    return RetVal::OK;
}

///////////////////////////////////////////////////////////////////////////////

using user::plist;

enum class commands : size_t {
    _, 
    禁烟,
    解禁,
    接近我,
};
const std::vector<std::pair<std::regex, commands>> group_commands_regex
{
    {std::regex("^禁(言|烟|菸|煙)([^\\s]*) ([^\\s]*?)$", std::regex::optimize | std::regex::extended), commands::禁烟},
    {std::regex("^(接近|解禁)([^\\s]*) ([^\\s]*?)$", std::regex::optimize | std::regex::extended), commands::解禁},
};

const std::vector<std::pair<std::regex, commands>> private_commands_regex
{
    {std::regex("^(接近|解禁)(我)?$", std::regex::optimize | std::regex::extended), commands::接近我},
};

std::string nosmokingWrapper(int64_t qq, int64_t group, int64_t target, int64_t duration);
void 禁烟(const mirai::MsgMetadata& m, int64_t target_qqid, int64_t duration)
{
    if (grp::groups[m.groupid].haveMember(botLoginQQId))
        if (grp::groups[m.groupid].members[botLoginQQId].permission == mirai::group_member_permission::MEMBER) 
            return;

    if (plist.find(m.qqid) == plist.end())
    {
        mirai::sendGroupMsgStr(m.groupid, "没开户就想烟人？");
        return;
    }

    if (duration == LLONG_MIN || duration == LLONG_MAX)
    {
        mirai::sendGroupMsgStr(m.groupid, "你会不会烟人？");
        return;
    }
    
    if (duration < 0)
    {
        mirai::sendGroupMsgStr(m.groupid, "您抽烟倒着抽？");
        return;
    }

    // smoke last member if req without target
    int64_t target = 0;
    if (target_qqid == 0)
        target = grp::groups[m.groupid].last_talk_member;
    else
        target = target_qqid;

    mirai::sendGroupMsgStr(m.groupid, nosmokingWrapper(m.qqid, m.groupid, target, duration));
    return;
}

json not_enough_currency(int64_t qq, int64_t cost)
{
    json resp = R"({ "messageChain": [] })"_json;
    resp["messageChain"].push_back(mirai::buildMessageAt(qq));
    std::stringstream ss;
    ss << "，你的余额不足，需要" << cost << "个批";
    resp["messageChain"].push_back(mirai::buildMessagePlain(ss.str()));
    return resp;
}

std::string nosmokingWrapper(int64_t qq, int64_t group, int64_t target, int64_t duration)
{
    if (duration > 30 * 24 * 60) duration = 30 * 24 * 60;
    if (duration == 0)
    {
        nosmoking(group, target, duration);
        return "解禁了";
    }

    int cost = (int64_t)std::floor(std::pow(duration, 1.777777));
    if (cost > plist[qq].getCurrency())
    {
        mirai::sendGroupMsg(group, not_enough_currency(qq, cost));
    }

    double reflect = randReal();

    // 10% fail
    if (reflect < 0.1) 
        return "烟突然灭了，烟起失败";

    // 20% reflect
    else if (reflect < 0.3)
    {
        switch (nosmoking(group, qq, duration))
        {
            using r = RetVal;

        case r::OK:
            return "你自己烟起吧";
        case r::TARGET_IS_ADMIN:
            return "烟突然灭了，烟起失败";
        case r::TARGET_NOT_FOUND:
            return "瞎几把烟谁呢";
        default:
            return "你会不会烟人？";
        }
    }
    else
    {
        switch (nosmoking(group, target, duration))
        {
            using r = RetVal;

        case r::OK:
        {
            plist[qq].modifyCurrency(-cost);
            std::stringstream ss;
            if (qq == target)
                ss << "？我从来没听过这样的要求，消费" << cost << "个批";
            else
                ss << "烟起哦，消费" << cost << "个批";
            return ss.str();
        }

        case r::TARGET_IS_ADMIN:
            return "禁烟管理请联系群主哦";

        case r::TARGET_NOT_FOUND:
            return "瞎几把烟谁呢";

        default:
            return "你会不会烟人？";
        }
    }
}

void 群聊解禁(const mirai::MsgMetadata& m, int64_t target_qqid)
{
    if (grp::groups[m.groupid].haveMember(botLoginQQId))
        if (grp::groups[m.groupid].members[botLoginQQId].permission == mirai::group_member_permission::MEMBER) 
            return;

    if (target_qqid == 0)
    {
        mirai::sendGroupMsgStr(m.groupid, "查无此人");
        return;
    }

    if (RetVal::OK == nosmoking(m.groupid, target_qqid, 0))
        mirai::sendGroupMsgStr(m.groupid, "解禁了");
    else
        mirai::sendGroupMsgStr(m.groupid, "解禁失败");
}

void groupMsgCallback(const json& body)
{
    auto query = mirai::messageChainToStr(body);
    if (query.empty()) return;

    commands c = commands::_;
    int64_t target_qqid = 0;
    int64_t duration = 0;
    for (const auto& [re, cmd]: group_commands_regex)
    {
        std::smatch res;
        if (std::regex_match(query, res, re))
        {
            c = cmd;
            target_qqid = res[2].str().empty() ? 0 : user::getUser(res[2].str());
            duration = std::strtoll(res[3].str().c_str(), nullptr, 10);
            break;
        }
    }
    if (c == commands::_) return;

    auto m = mirai::parseMsgMetadata(body);
    grp::newGroupIfNotExist(m.groupid);

    switch (c)
    {
    case commands::禁烟:
        禁烟(m, target_qqid, duration);
        break;
    case commands::解禁:
        群聊解禁(m, target_qqid);
        break;
    default:
        break;
    }
}

std::string selfUnsmoke(int64_t qq)
{
    if (smokeTimeInGroups.find(qq) == smokeTimeInGroups.end() || smokeTimeInGroups[qq].empty())
        return "你么得被烟啊";

    if (plist.find(qq) == plist.end()) return "你还没有开通菠菜";
    auto &p = plist[qq];

    time_t t = time(nullptr);
    int64_t total_remain = 0;
    for (auto& g : smokeTimeInGroups[qq])
    {
        if (t <= g.second)
        {
            int64_t remain = (g.second - t) / 60; // min
            int64_t extra = (g.second - t) % 60;  // sec
            total_remain += remain + !!extra;
        }
    }
    std::stringstream ss;
    ss << "你在" << smokeTimeInGroups[qq].size() << "个群被烟" << total_remain << "分钟，";
    int64_t total_cost = (int64_t)std::ceil(total_remain * UNSMOKE_COST_RATIO);
    if (p.getCurrency() < total_cost)
    {
        ss << "你批不够，需要" << total_cost << "个批，哈";
    }
    else
    {
        p.modifyCurrency(-total_cost);
        ss << "本次接近消费" << total_cost << "个批";

        // broadcast to groups
        for (auto& g : smokeTimeInGroups[qq])
        {
            if (t > g.second) continue;

            mirai::unmute(qq, g.first);

            std::string qqname = mirai::getGroupMemberCard(g.first, qq);
            if (qqname.empty()) qqname = std::to_string(qq);
            int64_t remain = (g.second - t) / 60; // min
            int64_t extra = (g.second - t) % 60;  // sec

            std::stringstream sg;
            sg << qqname << "花费巨资" << (int64_t)std::round((remain + !!extra) * UNSMOKE_COST_RATIO) << "个批申请接近成功";
            mirai::sendGroupMsgStr(g.first, sg.str());
        }
        smokeTimeInGroups[qq].clear();
    }
    return ss.str();
}

void privateMsgCallback(const json& body)
{
    auto query = mirai::messageChainToStr(body);
    if (query.empty()) return;

    commands c = commands::_;
    int64_t target_qqid = 0;
    int64_t duration = 0;
    for (const auto& [re, cmd]: private_commands_regex)
    {
        std::smatch res;
        if (std::regex_match(query, res, re))
        {
            c = cmd;
            target_qqid = user::getUser(res[2].str());
            duration = std::strtoll(res[3].str().c_str(), nullptr, 10);
            break;
        }
    }
    if (c == commands::_) return;

    auto m = mirai::parseMsgMetadata(body);

    std::string resp;
    switch (c)
    {
    case commands::接近我:
        resp = selfUnsmoke(m.qqid);
        break;
    default:
        break;
    }

    mirai::sendMsgRespStr(m, resp);
}

}