#include "monopoly.h"

#include <filesystem>
#include <functional>
#include <map>
#include <list>

#include <boost/algorithm/string.hpp>

#include "yaml-cpp/yaml.h"

#include "data/group.h"
#include "data/user.h"
#include "smoke.h"
#include "utils/logger.h"
#include "utils/rand.h"

#include "mirai/api.h"
#include "mirai/msg.h"
#include "mirai/util.h"

namespace monopoly
{

using namespace std::placeholders;

using func = std::function<void(int64_t qqid, int64_t groupid, int64_t& target)>;

struct chance
{
    double prob;
    std::string msg;
    int category;
    std::list<func> cmds;
    static double total_prob;
};
double chance::total_prob = 0;
std::vector<chance> chanceList;

struct flags
{
    time_t  freeze_assets_expire_time = 0;
    time_t  adrenaline_expire_time = LONG_MAX;
    bool    chaos = false;
    bool    skip_mul_all_currency_sub1 = false;

};
std::map<int64_t, flags> user_stat;

namespace command
{
inline int64_t round(double x) { return static_cast<int64_t>(std::round(x)); }
inline int64_t trunc(double x) { return static_cast<int64_t>(std::trunc(x)); }


using efunc = std::function<void(int64_t qqid, int64_t groupid, double x, double y)>;
void do_all(int64_t groupid, efunc f, double x = 0.0, double y = 0.0);

int64_t get_random_registered_member(int64_t groupid);

// events
auto& pd = user::plist;
void muted(int64_t qqid, int64_t groupid, double x) { smoke::nosmoking(groupid, qqid, round(x * 60)); }
void mute_dk(int64_t qqid, int64_t groupid, double x) { smoke::nosmoking(groupid, /*TODO who is dk*/0, round(x * 60)); }
void mute_bot(int64_t qqid, int64_t groupid, double x) { smoke::nosmoking(groupid, botLoginQQId, round(x * 60)); }
void mute_random(int64_t qqid, int64_t groupid, double x, int64_t& target) { target = get_random_registered_member(groupid); smoke::nosmoking(groupid, target, round(x * 60)); }
void give_key(int64_t qqid, double x) { pd[qqid].modifyKeyCount(round(x)); }
void give_currency(int64_t qqid, double x) { pd[qqid].modifyCurrency(round(x)); }
void give_stamina(int64_t qqid, double x) { pd[qqid].modifyStamina(-round(x)); }
void give_stamina_extra(int64_t qqid, double x) { pd[qqid].modifyStamina(-round(x), true); }
void set_stamina(int64_t qqid, double x) { pd[qqid].modifyStamina(pd[qqid].modifyStamina(0).staminaAfterUpdate); pd[qqid].modifyStamina(-round(x)); }
void mul_currency(int64_t qqid, double x) { pd[qqid].modifyCurrency(round(pd[qqid].getCurrency() * x) - pd[qqid].getCurrency()); }
void give_all_key(int64_t groupid, double x) { do_all(groupid, std::bind(give_key, _1, _3), x); }
void give_all_currency(int64_t groupid, double x) { do_all(groupid, std::bind(give_currency, _1, _3), x); }
void give_all_currency_range(int64_t groupid, double x, double y) { do_all(groupid, [](int64_t qqid, int64_t groupid, double x, double y) {pd[qqid].modifyCurrency(randInt(round(x), round(y))); }, x, y); }
void give_all_stamina(int64_t groupid, double x) { do_all(groupid, std::bind(give_stamina, _1, _3), x); }
void mul_all_currency(int64_t groupid, double x) { do_all(groupid, std::bind(mul_currency, _1, _3), x); }
void set_all_stamina(int64_t groupid, double x) { do_all(groupid, std::bind(set_stamina, _1, _3), x); }
void add_daily_pool(double x) { user::extra_tomorrow += round(x); }
void get_mul_sub1_skip(int64_t qqid) { user_stat[qqid].skip_mul_all_currency_sub1 = true; }
void fever(int64_t qqid, double x) { user_stat[qqid].adrenaline_expire_time = time(nullptr) + 15; }
void chaos(int64_t qqid) { user_stat[qqid].chaos = true; }

const std::map<std::string, void*> strMap
{
    {"muted", (void*)muted},
    {"mute_dk", (void*)mute_dk},
    {"mute_bot", (void*)mute_bot},
    {"mute_random", (void*)mute_random},
    {"give_key", (void*)give_key},
    {"give_currency", (void*)give_currency},
    {"give_stamina", (void*)give_stamina},
    {"give_stamina_extra", (void*)give_stamina_extra},
    {"set_stamina", (void*)set_stamina},
    {"mul_currency", (void*)mul_currency},
    {"give_all_key", (void*)give_all_key},
    {"give_all_currency", (void*)give_all_currency},
    {"give_all_currency_range", (void*)give_all_currency_range},
    {"give_all_stamina", (void*)give_all_stamina},
    {"mul_all_currency", (void*)mul_all_currency},
    {"set_all_stamina", (void*)set_all_stamina},
    {"add_daily_pool", (void*)add_daily_pool},
    {"get_mul_sub1_skip", (void*)get_mul_sub1_skip},
    {"fever", (void*)fever},
    {"chaos", (void*)chaos},
};

void do_all(int64_t groupid, efunc f, double x, double y)
{
    bool check_skip_mul_all_currency_sub1 = false;
    {
        auto pf = f.target<decltype(mul_all_currency)*>();
        if (pf && *pf == mul_all_currency && x < 1.0)
            check_skip_mul_all_currency_sub1 = true;
    }

    for (auto& [qqid, u] : grp::groups.at(groupid).members)
    {
        if (check_skip_mul_all_currency_sub1)
        {
            if (user_stat[qqid].skip_mul_all_currency_sub1)
            {
                user_stat[qqid].skip_mul_all_currency_sub1 = false;
                continue;
            }
        }

        f(qqid, groupid, x, y);
    }
}

int64_t get_random_registered_member(int64_t groupid)
{
    std::vector<int64_t> registeredMemberList;
    for (auto& [qq, pd] : user::plist)
    {
        if (grp::groups[groupid].haveMember(qq))
            registeredMemberList.push_back(qq);
    }
    if (!registeredMemberList.empty())
    {
        size_t idx = randInt(0, registeredMemberList.size() - 1);
        return registeredMemberList[idx];
    }
    return 0;
}

}

int init(const char* yaml)
{
    std::filesystem::path cfgPath(yaml);
    if (!std::filesystem::is_regular_file(cfgPath))
    {
        addLog(LOG_ERROR, "monopoly", "chance list file %s not found", std::filesystem::absolute(cfgPath).c_str());
        return -1;
    }
    addLog(LOG_INFO, "monopoly", "Loading chance events from %s", std::filesystem::absolute(cfgPath).c_str());

    YAML::Node cfg = YAML::LoadFile(yaml);
    chance::total_prob = 0.0;
    for (const auto& u : cfg)
    {
        chance c;
        c.prob = u["prob"].as<double>();
        c.msg = u["msg"].as<std::string>();
        for (const auto& e : u["cmds"])
        {
            std::vector<std::string> args;
            boost::algorithm::split(args, e.as<std::string>(), boost::is_any_of(" "), boost::algorithm::token_compress_on);
            if (args.empty()) break;

            auto& cmd = args[0];
            if (command::strMap.find(cmd) != command::strMap.end())
            {
                double x = 0.0, y = 0.0;
                if (args.size() >= 2)
                {
                    char* endptr = nullptr;
                    double tmp = std::strtod(args[1].c_str(), &endptr);
                    if (!endptr)
                    {
                        addLog(LOG_INFO, "monopoly", "error parsing param \"%s\"", e.as<std::string>().c_str());
                        continue;
                    }
                    x = tmp;
                }
                if (args.size() >= 3)
                {
                    char* endptr = nullptr;
                    double tmp = std::strtod(args[2].c_str(), &endptr);
                    if (!endptr)
                    {
                        addLog(LOG_INFO, "monopoly", "error parsing param \"%s\"", e.as<std::string>().c_str());
                        continue;
                    }
                    y = tmp;
                }

                using namespace command;
                auto pf = (uintptr_t)strMap.at(cmd);
                if      (pf == (uintptr_t)muted)                c.cmds.emplace_back(std::bind(muted, _1, _2, x));
                else if (pf == (uintptr_t)mute_dk)              c.cmds.emplace_back(std::bind(mute_dk, _1, _2, x));
                else if (pf == (uintptr_t)mute_bot)             c.cmds.emplace_back(std::bind(mute_bot, _1, _2, x));
                else if (pf == (uintptr_t)mute_random)          c.cmds.emplace_back(std::bind(mute_random, _1, _2, x, _3));
                else if (pf == (uintptr_t)give_key)             c.cmds.emplace_back(std::bind(give_key, _1, x));
                else if (pf == (uintptr_t)give_currency)        c.cmds.emplace_back(std::bind(give_currency, _1, x));
                else if (pf == (uintptr_t)give_stamina)         c.cmds.emplace_back(std::bind(give_stamina, _1, x));
                else if (pf == (uintptr_t)give_stamina_extra)   c.cmds.emplace_back(std::bind(give_stamina_extra, _1, x));
                else if (pf == (uintptr_t)set_stamina)          c.cmds.emplace_back(std::bind(set_stamina, _1, x));
                else if (pf == (uintptr_t)mul_currency)         c.cmds.emplace_back(std::bind(mul_currency, _1, x));
                else if (pf == (uintptr_t)give_all_key)         c.cmds.emplace_back(std::bind(give_all_key, _2, x));
                else if (pf == (uintptr_t)give_all_currency)    c.cmds.emplace_back(std::bind(give_all_currency, _2, x));
                else if (pf == (uintptr_t)give_all_currency_range) c.cmds.emplace_back(std::bind(give_all_currency_range, _2, x, y));
                else if (pf == (uintptr_t)give_all_stamina)     c.cmds.emplace_back(std::bind(give_all_stamina, _2, x));
                else if (pf == (uintptr_t)mul_all_currency)     c.cmds.emplace_back(std::bind(mul_all_currency, _2, x));
                else if (pf == (uintptr_t)set_all_stamina)      c.cmds.emplace_back(std::bind(set_all_stamina, _2, x));
                else if (pf == (uintptr_t)add_daily_pool)       c.cmds.emplace_back(std::bind(add_daily_pool, x));
                else if (pf == (uintptr_t)get_mul_sub1_skip)    c.cmds.emplace_back(std::bind(get_mul_sub1_skip, _1));
                else if (pf == (uintptr_t)fever)                c.cmds.emplace_back(std::bind(fever, _1, x));
                else if (pf == (uintptr_t)chaos)                c.cmds.emplace_back(std::bind(chaos, _1));
                else addLog(LOG_WARNING, "monopoly", "unknown event \"%s\"", cmd.c_str());
            }
            else
                addLog(LOG_WARNING, "monopoly", "unknown event \"%s\"", cmd.c_str());
        }
        c.total_prob += c.prob;
        chanceList.push_back(std::move(c));
    }
    addLog(LOG_INFO, "monopoly", "Loaded %u entries", chanceList.size());

    return 0;
}

std::string convertRespMsg(const std::string& raw, int64_t qqid, int64_t groupid, int64_t target, const user::pdata& pre, const user::pdata& post)
{
    std::string s;
    s += "，恭喜你抽到了" + raw;
    boost::algorithm::replace_all(s, "{currency_pre}",      std::to_string(pre.getCurrency()));
    boost::algorithm::replace_all(s, "{currency_post}",     std::to_string(post.getCurrency()));
    boost::algorithm::replace_all(s, "{currency_delta}",    std::to_string(post.getCurrency() - pre.getCurrency()));
    boost::algorithm::replace_all(s, "{key_pre}",           std::to_string(pre.getKeyCount()));
    boost::algorithm::replace_all(s, "{key_post}",          std::to_string(post.getKeyCount()));
    boost::algorithm::replace_all(s, "{key_delta}",         std::to_string(post.getKeyCount() - pre.getKeyCount()));
    boost::algorithm::replace_all(s, "{stamina_pre}",       std::to_string(pre.getStamina(true).staminaAfterUpdate));
    boost::algorithm::replace_all(s, "{stamina_post}",      std::to_string(post.getStamina(true).staminaAfterUpdate));
    boost::algorithm::replace_all(s, "{stamina_delta}",     std::to_string(post.getStamina(true).staminaAfterUpdate - pre.getStamina(true).staminaAfterUpdate));
    boost::algorithm::replace_all(s, "{player_card}",       grp::groups[groupid].members[qqid].nameCard);
    boost::algorithm::replace_all(s, "{target_card}",       target ? grp::groups[groupid].members[target].nameCard : "");
    return std::move(s);
}

void doChance(const mirai::MsgMetadata& m, const chance& c, bool adrenaline)
{
    // save pre-cmd info
    auto pre = user::plist[m.qqid];

    // run cmds
    int64_t target = 0;
    for (const auto& cmd : c.cmds)
    {
        cmd(m.qqid, m.groupid, target);
    }

    // send resp
    json resp = mirai::MSG_TEMPLATE;
    json& r = resp["messageChain"];
    if (adrenaline)
        r.push_back(mirai::buildMessagePlain("肾上腺素生效！"));
    r.push_back(mirai::buildMessageAt(m.qqid));
    r.push_back(mirai::buildMessagePlain(convertRespMsg(c.msg, m.qqid, m.groupid, target, pre, user::plist[m.qqid])));
    mirai::sendMsgResp(m, resp);

}

json not_registered(int64_t qq)
{
    json resp = mirai::MSG_TEMPLATE;
    resp["messageChain"].push_back(mirai::buildMessageAt(qq));
    resp["messageChain"].push_back(mirai::buildMessagePlain("，你还没有开通菠菜"));
    return resp;
}

json not_enough_stamina(int64_t qq, time_t rtime)
{
    json resp = mirai::MSG_TEMPLATE;
    resp["messageChain"].push_back(mirai::buildMessageAt(qq));
    std::stringstream ss;
    ss << "，你的体力不足，回满还需"
        << rtime / (60 * 60) << "小时" << rtime / 60 % 60 << "分钟";
    resp["messageChain"].push_back(mirai::buildMessagePlain(ss.str()));
    return resp;
}

void msgCallback(const json& body)
{
    auto query = mirai::messageChainToArgs(body);
    if (query.empty()) return;
    if (query[0] != "抽卡") return;

    auto m = mirai::parseMsgMetadata(body);

    if (!grp::groups[m.groupid].getFlag(grp::Group::MASK_P | grp::Group::MASK_MONOPOLY))
        return;

    if (user::plist.find(m.qqid) == user::plist.end()) 
    {
        mirai::sendMsgResp(m, not_registered(m.qqid));
        return;
    }

    time_t t = time(nullptr);
    bool adrenaline = t > user_stat[m.qqid].adrenaline_expire_time;
    if (!adrenaline)
    {
        auto [enough, stamina, rtime] = user::plist[m.qqid].modifyStamina(1);
        if (!enough)
        {
            mirai::sendMsgResp(m, not_enough_stamina(m.qqid, rtime));
            return;
        }
    }

    if (user_stat[m.qqid].chaos)
    {
        user_stat[m.qqid].chaos = false;
        size_t idx = randInt(0, chanceList.size() - 1);
        doChance(m, chanceList[idx], adrenaline);
    }
    else
    {
        auto prob = randReal(0, chance::total_prob);
        double p = 0.0;
        for (const auto& c : chanceList)
        {
            p += c.prob;
            if (prob < p)
            {
                doChance(m, c, adrenaline);
                break;
            }
        }
    }
}
}