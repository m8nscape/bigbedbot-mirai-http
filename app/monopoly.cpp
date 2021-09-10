#include "monopoly.h"

#if __GNUC__ >= 8
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

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

std::map<int64_t, std::map<int64_t, time_t>> starveFloodCtrlExpire;
const int STARVE_FLOOD_CTRL_DURATION = 5; // introduce 5s cooldown if stamina runs out

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
std::vector<chance> chanceListChaos;

struct flags
{
    time_t  freeze_assets_expire_time = 0;
    time_t  fever_expire_time = 0;
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

// events
auto& pd = user::plist;
void muted(int64_t qqid, int64_t groupid, double x)
{
    smoke::nosmoking(groupid, qqid, round(x));
    grp::groups[groupid].sum_smoke += round(x);
}

void mute_dk(int64_t qqid, int64_t groupid, double x)
{
    smoke::nosmoking(groupid, /*TODO who is dk*/0, round(x));
    grp::groups[groupid].sum_smoke += round(x);
}

void mute_bot(int64_t qqid, int64_t groupid, double x)
{
    smoke::nosmoking(groupid, botLoginQQId, round(x));
}

void mute_random(int64_t qqid, int64_t groupid, double x, int64_t& target)
{
    target = get_random_registered_member(groupid);
    smoke::nosmoking(groupid, target, round(x));
    grp::groups[groupid].sum_smoke += round(x);
}

void give_key(int64_t qqid, double x)
{
    pd[qqid].modifyKeyCount(round(x));
}

void give_currency(int64_t qqid, int64_t groupid, double x)
{
    pd[qqid].modifyCurrency(round(x));
    grp::groups[groupid].sum_earned += round(x);
}

void give_stamina(int64_t qqid, double x)
{
    pd[qqid].modifyStamina(round(x));
}

void give_stamina_extra(int64_t qqid, double x)
{
    pd[qqid].modifyStamina(round(x), true);
}

void set_stamina(int64_t qqid, double x)
{
    int s = pd[qqid].modifyStamina(0).staminaAfterUpdate;
    pd[qqid].modifyStamina(-s);
    pd[qqid].modifyStamina(round(x));
}

void mul_currency(int64_t qqid, int64_t groupid, double x) 
{ 
    if (x < 1 && user_stat[qqid].skip_mul_all_currency_sub1)
    {
        user_stat[qqid].skip_mul_all_currency_sub1 = false;
        return;
    }
    int64_t c = pd[qqid].getCurrency(); 
    int d = round(c * x) - c; 
    pd[qqid].modifyCurrency(d); 
    if (d >= 0)
        grp::groups[groupid].sum_earned += d;
    else
        grp::groups[groupid].sum_spent += -d;
}

void give_all_key(int64_t groupid, double x)
{
    do_all(groupid, std::bind(give_key, _1, _3), x);
}

void give_all_currency(int64_t groupid, double x)
{
    do_all(groupid, std::bind(give_currency, _1, groupid, _3), x);
}

void give_all_currency_range(int64_t groupid, double x, double y)
{
    do_all(groupid, [](int64_t qqid, int64_t groupid, double x, double y)
    {
        give_currency(qqid, groupid, randInt(round(x), round(y)));
    },
     x, y);
}
void give_all_stamina(int64_t groupid, double x)
{
    do_all(groupid, std::bind(give_stamina, _1, _3), x);
}

void mul_all_currency(int64_t groupid, double x)
{
    do_all(groupid, std::bind(mul_currency, _1, groupid, _3), x);
}

void set_all_stamina(int64_t groupid, double x)
{
    do_all(groupid, std::bind(set_stamina, _1, _3), x);
}

void add_daily_pool(double x)
{
    user::extra_tomorrow += round(x);
}

void get_mul_sub1_skip(int64_t qqid)
{
    user_stat[qqid].skip_mul_all_currency_sub1 = true;
}

void fever(int64_t qqid, double x)
{
    user_stat[qqid].fever_expire_time = time(nullptr) + 15;
}

void chaos(int64_t qqid)
{
    user_stat[qqid].chaos = true;
}

void chaos_all(int64_t groupid)
{
    do_all(groupid, std::bind(chaos, _1));
}

void currency_digit_random(int64_t qqid)
{
    int64_t oldCurrency = pd[qqid].getCurrency();
    auto currencyStr = std::to_string(oldCurrency);
    for (char& c: currencyStr) c = randInt('0', '9');
    int64_t newCurrency = std::stoll(currencyStr);
    pd[qqid].modifyCurrency(newCurrency - oldCurrency);
}

void currency_digit_reverse(int64_t qqid)
{
    int64_t oldCurrency = pd[qqid].getCurrency();
    auto currencyStr = std::to_string(oldCurrency);
    std::string newCurrencyStr;
    for (char& c: currencyStr) newCurrencyStr += c;
    int64_t newCurrency = std::stoll(newCurrencyStr);
    pd[qqid].modifyCurrency(newCurrency - oldCurrency);
}

void currency_digit_unify(int64_t qqid)
{
    int64_t oldCurrency = pd[qqid].getCurrency();
    auto currencyStr = std::to_string(oldCurrency);
    int i = randInt(int(currencyStr.length()) - 1);
    std::string newCurrencyStr(currencyStr.size(), currencyStr[i]);
    int64_t newCurrency = std::stoi(newCurrencyStr);
    pd[qqid].modifyCurrency(newCurrency - oldCurrency);
}

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
    {"chaos_all", (void*)chaos_all},
    {"currency_digit_random", (void*)currency_digit_random},
    {"currency_digit_reverse", (void*)currency_digit_reverse},
    {"currency_digit_unify", (void*)currency_digit_unify},
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
        if (user::plist.find(qqid) != user::plist.end())
            f(qqid, groupid, x, y);
    }
}


}

int init(const char* yaml)
{
    fs::path cfgPath(yaml);
    if (!fs::is_regular_file(cfgPath))
    {
        addLog(LOG_ERROR, "monopoly", "chance list file %s not found", fs::absolute(cfgPath).c_str());
        return -1;
    }
    addLog(LOG_INFO, "monopoly", "Loading chance events from %s", fs::absolute(cfgPath).c_str());

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
                // _1: qqid | _2: groupid
                auto pf = (uintptr_t)strMap.at(cmd);
                if      (pf == (uintptr_t)muted)                c.cmds.emplace_back(std::bind(muted, _1, _2, x));
                else if (pf == (uintptr_t)mute_dk)              c.cmds.emplace_back(std::bind(mute_dk, _1, _2, x));
                else if (pf == (uintptr_t)mute_bot)             c.cmds.emplace_back(std::bind(mute_bot, _1, _2, x));
                else if (pf == (uintptr_t)mute_random)          c.cmds.emplace_back(std::bind(mute_random, _1, _2, x, _3));
                else if (pf == (uintptr_t)give_key)             c.cmds.emplace_back(std::bind(give_key, _1, x));
                else if (pf == (uintptr_t)give_currency)        c.cmds.emplace_back(std::bind(give_currency, _1, _2, x));
                else if (pf == (uintptr_t)give_stamina)         c.cmds.emplace_back(std::bind(give_stamina, _1, x));
                else if (pf == (uintptr_t)give_stamina_extra)   c.cmds.emplace_back(std::bind(give_stamina_extra, _1, x));
                else if (pf == (uintptr_t)set_stamina)          c.cmds.emplace_back(std::bind(set_stamina, _1, x));
                else if (pf == (uintptr_t)mul_currency)         c.cmds.emplace_back(std::bind(mul_currency, _1, _2, x));
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
                else if (pf == (uintptr_t)chaos_all)            c.cmds.emplace_back(std::bind(chaos_all, _2));
                else if (pf == (uintptr_t)currency_digit_random) c.cmds.emplace_back(std::bind(currency_digit_random, _1));
                else if (pf == (uintptr_t)currency_digit_reverse) c.cmds.emplace_back(std::bind(currency_digit_reverse, _1));
                else if (pf == (uintptr_t)currency_digit_unify) c.cmds.emplace_back(std::bind(currency_digit_unify, _1));
                else addLog(LOG_WARNING, "monopoly", "unknown event \"%s\"", cmd.c_str());
            }
            else
                addLog(LOG_WARNING, "monopoly", "unknown event \"%s\"", cmd.c_str());
        }
        c.total_prob += c.prob;
        chanceList.push_back(std::move(c));
    }
    addLog(LOG_INFO, "monopoly", "Loaded %u entries", chanceList.size());

    std::sort(chanceList.begin(), chanceList.end(), [](const chance& l, const chance& r) { return l.prob > r.prob; });
    addLog(LOG_INFO, "monopoly", "Events sorted by probability");

    // generate chaos list (do not add first 20% entries)
    for (size_t i = chanceList.size() / 5; i < chanceList.size(); ++i)
    {
        chanceListChaos.push_back(chanceList[i]);
    }

    return 0;
}

std::string convertRespMsg(const std::string& raw, int64_t qqid, int64_t groupid, int64_t target, const user::pdata& pre, const user::pdata& post)
{
    std::string s;
    s += "，恭喜你抽到了" + raw;
    boost::algorithm::replace_all(s, "{currency_pre}",      std::to_string(pre.getCurrency()));
    boost::algorithm::replace_all(s, "{currency_post}",     std::to_string(post.getCurrency()));
    boost::algorithm::replace_all(s, "{key_pre}",           std::to_string(pre.getKeyCount()));
    boost::algorithm::replace_all(s, "{key_post}",          std::to_string(post.getKeyCount()));
    boost::algorithm::replace_all(s, "{key_delta}",         std::to_string(post.getKeyCount() - pre.getKeyCount()));
    boost::algorithm::replace_all(s, "{stamina_pre}",       std::to_string(pre.getStamina(true).staminaAfterUpdate));
    boost::algorithm::replace_all(s, "{stamina_post}",      std::to_string(post.getStamina(true).staminaAfterUpdate));
    boost::algorithm::replace_all(s, "{stamina_delta}",     std::to_string(post.getStamina(true).staminaAfterUpdate - pre.getStamina(true).staminaAfterUpdate));
    boost::algorithm::replace_all(s, "{player_card}",       grp::groups[groupid].members[qqid].nameCard);
    boost::algorithm::replace_all(s, "{target_card}",       target ? grp::groups[groupid].members[target].nameCard : "");

    using namespace std::string_literals;
    int64_t deltaCurrency = post.getCurrency() - pre.getCurrency();
    if (deltaCurrency > 0)
        boost::algorithm::replace_all(s, "{currency_delta_pos}", "+"s + std::to_string(deltaCurrency));
    else if (deltaCurrency == 0)
        boost::algorithm::replace_all(s, "{currency_delta_pos}", "+-0"s);
    else
        boost::algorithm::replace_all(s, "{currency_delta_pos}", std::to_string(deltaCurrency));

    boost::algorithm::replace_all(s, "{currency_delta}",    std::to_string(deltaCurrency));
    boost::algorithm::replace_all(s, "{currency_delta_neg}",std::to_string(-deltaCurrency));
    
    return std::move(s);
}

void doChance(const mirai::MsgMetadata& m, const chance& c, bool isInFever)
{
    // save pre-cmd info
    auto pre = user::plist[m.qqid];

    // run cmds
    int64_t target = 0;
    for (const auto& cmd : c.cmds)
    {
        cmd(m.qqid, m.groupid, target);
    }
    grp::groups[m.groupid].sum_card += 1;

    // send resp
    json resp = mirai::MSG_TEMPLATE;
    json& r = resp["messageChain"];
    if (isInFever)
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

    // stamina check
    time_t t = time(nullptr);
    bool isInFever = t <= user_stat[m.qqid].fever_expire_time;
    if (!isInFever)
    {
        auto [enough, stamina, rtime] = user::plist[m.qqid].modifyStamina(-1);
        if (enough)
        {
            starveFloodCtrlExpire[m.groupid][m.qqid] = 0;
        }
        else
        {
            // flood control
            if (starveFloodCtrlExpire[m.groupid][m.qqid] == 0 || starveFloodCtrlExpire[m.groupid][m.qqid] < t)
            {
                mirai::sendMsgResp(m, not_enough_stamina(m.qqid, rtime));
                starveFloodCtrlExpire[m.groupid][m.qqid] = t + STARVE_FLOOD_CTRL_DURATION;
            }
            return;
        }
    }

    if (user_stat[m.qqid].chaos)
    {
        user_stat[m.qqid].chaos = false;
        size_t idx = randInt(0, chanceListChaos.size() - 1);
        doChance(m, chanceListChaos[idx], isInFever);
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
                doChance(m, c, isInFever);
                break;
            }
        }
    }
}

// draft
void choukasuoha(const json& body)
{
    auto query = mirai::messageChainToArgs(body);
    if (query.empty()) return;
    if (query[0] != "抽卡梭哈") return;

    auto m = mirai::parseMsgMetadata(body);

    if (!grp::groups[m.groupid].getFlag(grp::Group::MASK_P | grp::Group::MASK_MONOPOLY))
        return;

    if (user::plist.find(m.qqid) == user::plist.end()) 
    {
        mirai::sendMsgResp(m, not_registered(m.qqid));
        return;
    }

    std::thread([&]()
    {
        while (user::plist[m.qqid].testStamina(1).enough && !smoke::isSmoking(m.qqid, m.groupid))
        {
            time_t t = time(nullptr);
            bool isInFever = t <= user_stat[m.qqid].fever_expire_time;
            if (!isInFever)
            {
                auto [enough, stamina, rtime] = user::plist[m.qqid].modifyStamina(-1);
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
                doChance(m, chanceList[idx], isInFever);
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
                        doChance(m, c, isInFever);
                        break;
                    }
                }
            }

            // interval
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(200ms);
        }
    }).detach();

}
}