#include <sstream>

#if __GNUC__ >= 8
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

#include "case.h"
#include "data/group.h"
#include "mirai/api.h"
#include "mirai/util.h"
#include "mirai/msg.h"
#include "data/user.h"
#include "utils/rand.h"
#include "utils/logger.h"

#include "yaml-cpp/yaml.h"

namespace opencase
{
using user::plist;

static std::string cfg_path;

int FEE_PER_CASE = 5;
// 技能设定
int OPEN_RUN_SP1_COST = 255;
size_t SP1_TYPE = 0;
int OPEN_RUN_SP2_COST = 50;
int OPEN_RUN_SP2_STAMINA = 6;
size_t SP2_TYPE = 1;
int OPEN_RUN_ENDLESS_COST = 50;
int OPEN_RUN_ENDLESS_STAMINA = 7;

// case type: {name, probability}
std::vector<case_type> CASE_TYPES
{
    { "测试箱1", 0.5 },
    { "测试箱2", 0.5 },
};
//case_type CASE_DEFAULT{ "测试箱2", /*1,*/ 1.0 };

case_detail::case_detail() : _type_idx(CASE_TYPES.size()) {}
const case_type& case_detail::type() const { return CASE_TYPES[_type_idx]; }

std::vector<std::vector<case_detail>> CASE_POOL{{{0, "菠菜种子", 1}}, {{1, "菠菜叶子", 1}}};

int loadCfg(const char* yaml)
{
    fs::path cfgPath(yaml);
    if (!fs::is_regular_file(cfgPath))
    {
        addLog(LOG_ERROR, "case", "Case config file %s not found", fs::absolute(cfgPath).c_str());
        return -1;
    }
    addLog(LOG_INFO, "case", "Loading case config from %s", fs::absolute(cfgPath).c_str());

    YAML::Node cfg = YAML::LoadFile(yaml);

    // 配置
    FEE_PER_CASE = cfg["cost_draw"].as<int>(FEE_PER_CASE);
    OPEN_RUN_SP1_COST = cfg["open_run_sp1_cost"].as<int>(OPEN_RUN_SP1_COST);
    SP1_TYPE = cfg["sp1_type"].as<int>(SP1_TYPE);
    OPEN_RUN_SP2_COST = cfg["open_run_sp2_cost"].as<int>(OPEN_RUN_SP2_COST);
    OPEN_RUN_SP2_STAMINA = cfg["open_run_sp2_stamina"].as<int>(OPEN_RUN_SP2_STAMINA);
    SP2_TYPE = cfg["sp2_type"].as<int>(SP2_TYPE);
    OPEN_RUN_ENDLESS_COST = cfg["open_run_endless_cost"].as<int>(OPEN_RUN_ENDLESS_COST);
    OPEN_RUN_ENDLESS_STAMINA = cfg["open_run_endless_stamina"].as<int>(OPEN_RUN_ENDLESS_STAMINA);

    // 箱子等级列表
    CASE_TYPES.clear();
    double p = 0;
    for (const auto& it: cfg["levels"])
    {
        if (it.size() >= 2)
        {
            double pp = it[1].as<double>();
            CASE_TYPES.push_back({it[0].as<std::string>(), pp});
            p += pp;
        }
        else
        {
            CASE_TYPES.push_back({it[0].as<std::string>(), 1.0 - p});
            break;
        }
    }
    if (p > 1.0)
    {
        addLog(LOG_WARNING, "case", "sum of level probabilities %lf exceeds 1.0", p);
    }

    // 箱池
    CASE_POOL.clear();
    CASE_POOL.resize(CASE_TYPES.size());
    size_t case_count = 0;
    for (const auto& it: cfg["list"])
    {
        size_t level = it[0].as<size_t>();
        if (level < CASE_POOL.size())
            CASE_POOL[level].push_back({level, it[1].as<std::string>(), it[2].as<int>()});
        ++case_count;
    }

    addLog(LOG_INFO, "case", "Loaded %u cases", case_count);
    return 0;
}


json not_registered(int64_t qq)
{
    json resp = mirai::MSG_TEMPLATE;
    resp["messageChain"].push_back(mirai::buildMessageAt(qq));
    resp["messageChain"].push_back(mirai::buildMessagePlain("，你还没有开通菠菜"));
    return resp;
}

json not_enough_currency(int64_t qq)
{
    json resp = mirai::MSG_TEMPLATE;
    resp["messageChain"].push_back(mirai::buildMessageAt(qq));
    resp["messageChain"].push_back(mirai::buildMessagePlain("，你的余额不足"));
    return resp;
}

json not_enough_stamina(int64_t qq, time_t rtime)
{
    json resp = mirai::MSG_TEMPLATE;
    resp["messageChain"].push_back(mirai::buildMessageAt(qq));
    std::stringstream ss;
    ss << "，你的体力不足，回满还需" << rtime / (60 * 60) << "小时" << rtime / 60 % 60 << "分钟";
    resp["messageChain"].push_back(mirai::buildMessagePlain(ss.str()));
    return resp;
}

json OPEN_1(::int64_t group, ::int64_t qq, std::vector<std::string> args)
{
    if (plist.find(qq) == plist.end()) 
        return not_registered(qq);

    auto &p = plist[qq];
    //CQ_setGroupBan(ac, group, qq, 60);
    //return "不准开";

    if (p.getKeyCount() >= 1)
    {
        p.modifyKeyCount(-1);
    }
    else if (p.getCurrency() >= FEE_PER_CASE)
    {
        auto[enough, stamina, rtime] = p.testStamina(1);
        if (!enough)
            return not_enough_stamina(qq, rtime);
        p.modifyCurrency(-FEE_PER_CASE);
        grp::groups[group].sum_spent += FEE_PER_CASE;
        p.modifyStamina(-1, true);
    }
    else
        return not_enough_currency(qq);

    const case_detail& reward = draw_case(randReal());

    json resp = R"({ "messageChain": [] })"_json;
        json& r = resp["messageChain"];
    if (reward > 300)
    {
        r.push_back(mirai::buildMessagePlain("歪哟，"));
        r.push_back(mirai::buildMessageAt(qq));
        r.push_back(mirai::buildMessagePlain("发了，开出了"));
    }
    else 
    {
        r.push_back(mirai::buildMessageAt(qq));
        r.push_back(mirai::buildMessagePlain("，恭喜你开出了"));
    }
    std::stringstream ss;
    ss << reward.full() << "，获得" << reward.worth() << "个批";
    r.push_back(mirai::buildMessagePlain(ss.str()));

    p.modifyCurrency(reward.worth());
    grp::groups[group].sum_earned += reward.worth();
    p.modifyBoxCount(+1);
    grp::groups[group].sum_case += 1;
    //ss << "你还有" << stamina << "点体力，";

    return resp;
}

json OPEN_10(::int64_t group, ::int64_t qq, std::vector<std::string> args)
{
    if (plist.find(qq) == plist.end())
        return not_registered(qq);

    auto &p = plist[qq];
    //CQ_setGroupBan(ac, group, qq, 60);
    //return "不准开";

    if (p.getKeyCount() >= 10)
    {
        p.modifyKeyCount(-10);
    }
    else if (p.getCurrency() >= FEE_PER_CASE * 10)
    {
        auto[enough, stamina, rtime] = p.testStamina(10);
        if (!enough)
            return not_enough_stamina(qq, rtime);

        p.modifyCurrency(-FEE_PER_CASE * 10);
        grp::groups[group].sum_spent += FEE_PER_CASE * 10;
        p.modifyStamina(-10, true);
    }
    else
        return not_enough_currency(qq);

    std::vector<int> case_counts(CASE_TYPES.size(), 0);
    int rc = 0;
    /*
    for (size_t i = 0; i < 10; ++i)
    {
        const case_detail& reward = draw_case(randReal());
        case_counts[reward.type_idx()]++;
        r += reward.worth();
    }
    if (r > 300) ss << "歪哟，" << CQ_At(qq) << "发了，开出了";
    else ss << CQ_At(qq) << "，恭喜你开出了";
    for (size_t i = 0; i < case_counts.size(); ++i)
    {
        if (case_counts[i])
        {
            ss << case_counts[i] << "个" <<
                ((i == CASE_TYPES.size()) ? CASE_DEFAULT.name() : CASE_TYPES[i].name()) << "，";
        }
    }
    ss << "一共" << r << "个批";
    */

    json resp = R"({ "messageChain": [] })"_json;
    json& r = resp["messageChain"];
    r.push_back(mirai::buildMessageAt(qq));
    r.push_back(mirai::buildMessagePlain("，恭喜你开出了：\n"));
    for (size_t i = 0; i < 10; ++i)
    {
        const case_detail& reward = draw_case(randReal());
        case_counts[reward.type_idx()]++;
        rc += reward.worth();
        std::stringstream ss;
        ss << "- " << reward.full() << " (" << reward.worth() << "批)\n";
        r.push_back(mirai::buildMessagePlain(ss.str()));
    }
    
    std::stringstream ss;
    ss << "上面有";
    for (size_t i = 0; i < case_counts.size(); ++i)
    {
        if (case_counts[i])
        {
            ss << case_counts[i] << "个" <<
                //((i == CASE_TYPES.size()) ? CASE_DEFAULT.name() : CASE_TYPES[i].name()) << "，";
                CASE_TYPES[i].name() << "，";
        }
    }
    ss << "一共" << rc << "个批";
    r.push_back(mirai::buildMessagePlain(ss.str()));

    p.modifyCurrency(+rc);
    grp::groups[group].sum_earned += rc;
    p.modifyBoxCount(+10);
    grp::groups[group].sum_case += 10;

    return resp;
}

json OPEN_SP1(::int64_t group, ::int64_t qq, std::vector<std::string> args)
{
    if (plist.find(qq) == plist.end())
        return not_registered(qq);

    auto &p = plist[qq];
    //CQ_setGroupBan(ac, group, qq, 60);
    //return "不准开";

    if (p.getCurrency() < OPEN_RUN_SP1_COST)
        return not_enough_currency(qq);

    auto[enough, stamina, rtime] = p.testStamina(user::MAX_STAMINA);

    if (!enough)
        return not_enough_stamina(qq, rtime);
    
    p.modifyStamina(-user::MAX_STAMINA, true);

    json resp = R"({ "messageChain": [] })"_json;
    json& r = resp["messageChain"];
    std::stringstream ss;
    r.push_back(mirai::buildMessageAt(qq));
    ss << "消耗" << OPEN_RUN_SP1_COST << "个批和" << user::MAX_STAMINA << "点体力发动技能！\n";
    r.push_back(mirai::buildMessagePlain(ss.str()));
    ss.str("");

    std::vector<int> case_counts(CASE_TYPES.size(), 0);
    int count = 0;
    int cost = OPEN_RUN_SP1_COST;
    int res = 0;
    int64_t pee = p.getCurrency() - cost;
    case_detail reward;
    do {
        ++count;
        cost += FEE_PER_CASE;
        reward = draw_case(randReal());
        case_counts[reward.type_idx()]++;
        res += reward.worth();
        pee += reward.worth() - FEE_PER_CASE;
        if (reward.worth() >= 0)
            grp::groups[group].sum_earned += reward.worth();
        else
            grp::groups[group].sum_spent += -reward.worth();
        grp::groups[group].sum_spent += FEE_PER_CASE;
        grp::groups[group].sum_case += 1;

        if (reward.type_idx() == SP1_TYPE)
        {
            ss << grp::groups[group].getMemberName(qq)
                << "开了" << count << "个箱子终于开出了" << reward.full() << " (" << reward.worth() << "批)，"
                << "本次净收益" << pee - p.getCurrency() << "个批";
            r.push_back(mirai::buildMessagePlain(ss.str()));

            p.modifyCurrency(pee - p.getCurrency());
            p.modifyBoxCount(count);
            return resp;
        }
    } while (pee >= FEE_PER_CASE);


    ss << grp::groups[group].getMemberName(qq)
        << "破产了，开了" << count << "个箱子也没能开出黄箱，"
        << "本次净收益" << pee - p.getCurrency() << "个批";
    r.push_back(mirai::buildMessagePlain(ss.str()));

    p.modifyCurrency(pee - p.getCurrency());
    p.modifyBoxCount(count);
    return resp;

    //ss << "你还有" << stamina << "点体力，";
}

json OPEN_SP2(::int64_t group, ::int64_t qq, std::vector<std::string> args)
{
    if (plist.find(qq) == plist.end())
        return not_registered(qq);

    auto &p = plist[qq];
    //CQ_setGroupBan(ac, group, qq, 60);
    //return "不准开";

    if (p.getCurrency() < OPEN_RUN_SP2_COST)
        return not_enough_currency(qq);

    auto[enough, stamina, rtime] = p.testStamina(OPEN_RUN_SP2_STAMINA);

    if (!enough)
        return not_enough_stamina(qq, rtime);
    
    p.modifyStamina(-OPEN_RUN_SP2_STAMINA, true);

    json resp = R"({ "messageChain": [] })"_json;
    json& r = resp["messageChain"];
    std::stringstream ss;
    r.push_back(mirai::buildMessageAt(qq));
    ss << "消耗" << OPEN_RUN_SP2_COST << "个批和" << OPEN_RUN_SP2_STAMINA << "点体力发动技能！\n";
    r.push_back(mirai::buildMessagePlain(ss.str()));
    ss.str("");

    std::vector<int> case_counts(CASE_TYPES.size(), 0);
    int count = 0;
    int cost = OPEN_RUN_SP2_COST;
    int res = 0;
    int64_t pee = p.getCurrency() - cost;
    case_detail reward;
    do {
        ++count;
        cost += FEE_PER_CASE;
        reward = draw_case(randReal());
        case_counts[reward.type_idx()]++;
        res += reward.worth();
        pee += reward.worth() - FEE_PER_CASE;
        if (reward.worth() >= 0)
            grp::groups[group].sum_earned += reward.worth();
        else
            grp::groups[group].sum_spent += -reward.worth();
        grp::groups[group].sum_spent += FEE_PER_CASE;
        grp::groups[group].sum_case += 1;

        if (reward.type_idx() == SP2_TYPE)
        {
            ss << grp::groups[group].getMemberName(qq)
                << "开了" << count << "个箱子终于开出了" << reward.full() << " (" << reward.worth() << "批)，"
                << "本次净收益" << pee - p.getCurrency() << "个批";
            r.push_back(mirai::buildMessagePlain(ss.str()));

            p.modifyCurrency(pee - p.getCurrency());
            p.modifyBoxCount(count);
            return resp;
        }
        else if (reward.type_idx() == SP1_TYPE)
        {
            r.push_back(mirai::buildMessagePlain("歪哟，"));
            r.push_back(mirai::buildMessageAt(qq));
            ss << "发了，开了" << count << "个箱子居然开出了" << reward.full() << " (" << reward.worth() << "批)，"
                << "本次净收益" << pee - p.getCurrency() << "个批";
            r.push_back(mirai::buildMessagePlain(ss.str()));

            p.modifyCurrency(pee - p.getCurrency());
            p.modifyBoxCount(count);
            return resp;
        }
    } while (pee >= FEE_PER_CASE);


    ss << grp::groups[group].getMemberName(qq)
        << "破产了，开了" << count << "个箱子也没能开出红箱，"
        << "本次净收益" << pee - p.getCurrency() << "个批";
    r.push_back(mirai::buildMessagePlain(ss.str()));

    p.modifyCurrency(pee - p.getCurrency());
    p.modifyBoxCount(count);
    return resp;

    //ss << "你还有" << stamina << "点体力，";
}

json OPEN_ENDLESS(::int64_t group, ::int64_t qq, std::vector<std::string> args)
{
    json resp = mirai::MSG_TEMPLATE;
    json& r = resp["messageChain"];
    r.push_back(mirai::buildMessagePlain("梭哈台被群主偷了，没得梭了"));
    return resp;

    /*
case commands::开箱endless:
    c.func = [](::int64_t group, ::int64_t qq, std::vector<std::string> args) -> std::string
    {
        if (plist.find(qq) == plist.end()) return std::string(CQ_At(qq)) + "，你还没有开通菠菜";
        //CQ_setGroupBan(ac, group, qq, 60);
        //return "不准开";

        if (plist[qq].currency < OPEN_RUN_ENDLESS_COST)
            return std::string(CQ_At(qq)) + "，你的余额不足";

        auto [enough, stamina, rtime] = updateStamina(qq, OPEN_RUN_ENDLESS_STAMINA);

        std::stringstream ss;
        if (!enough) ss << CQ_At(qq) << "，你的体力不足，回满还需"
            << rtime / (60 * 60) << "小时" << rtime / 60 % 60 << "分钟";
        else
        {
            plist[qq].currency -= OPEN_RUN_ENDLESS_COST;
            long long total_cases = plist[qq].currency / FEE_PER_CASE;
            plist[qq].currency %= FEE_PER_CASE;
            long long extra_cost = 0.1 * total_cases * FEE_PER_CASE;
            total_cases -= long long(std::floor(total_cases * 0.1));
            ss << getCard(group, qq) << "消耗" << OPEN_RUN_ENDLESS_COST + extra_cost << "个批和" << OPEN_RUN_ENDLESS_STAMINA << "点体力发动技能！\n";
            std::vector<int> case_counts(CASE_TYPES.size() + 1, 0);
            int count = 0;
            int cost = 0;
            int res = 0;
            case_detail max;
            case_detail reward;
            do {
                ++count;
                cost += FEE_PER_CASE;
                reward = draw_case(randReal());
                case_counts[reward.type_idx()]++;
                res += reward.worth();
                plist[qq].currency += reward.worth();
                plist[qq].opened_box_count++;

                if (max.worth() < reward.worth()) max = reward;
            } while (--total_cases > 0);

            ss << CQ_At(qq) << "本次梭哈开了" << count << "个箱子，开出了" << case_counts[1] << "个黄箱，" << case_counts[2] << "个红箱，" << case_counts[0] << "个黑箱，\n"
                << "最值钱的是" << max.full() << " (" << max.worth() << "批)，"
                << "本次净收益" << res - cost - OPEN_RUN_ENDLESS_COST - extra_cost << "个批";
            if (plist[qq].currency < 0) plist[qq].currency = 0;
            modifyCurrency(qq, plist[qq].currency);
            modifyBoxCount(qq, plist[qq].opened_box_count);
            return ss.str();

            //ss << "你还有" << stamina << "点体力，";
        }

        return ss.str();
    };
    break;
    */
}
   
json RELOAD(::int64_t group, ::int64_t qq, std::vector<std::string> args)
{
    if (!grp::checkPermission(group, qq, mirai::group_member_permission::ROOT, true))
        return json();

    loadCfg(cfg_path.c_str());

    json resp = mirai::MSG_TEMPLATE;
    json& r = resp["messageChain"];
    r.push_back(mirai::buildMessagePlain("好"));
    return resp;
}

enum class commands : size_t {
	OPEN_1,
	OPEN_10,
	OPEN_SP1,
	OPEN_SP2,
	OPEN_ENDLESS,
    RELOAD,
};

const std::map<std::string, commands> commands_str
{
    {"开箱", commands::OPEN_1},
    {"開箱", commands::OPEN_1},  //繁體化
    {"开箱十连", commands::OPEN_10},
    {"開箱十連", commands::OPEN_10},  //繁體化
    {"开黄箱", commands::OPEN_SP1},
    {"開黃箱", commands::OPEN_SP1},  //繁體化
    {"开红箱", commands::OPEN_SP2},
    {"開紅箱", commands::OPEN_SP2},  //繁體化
    {"开箱梭哈", commands::OPEN_ENDLESS},
    {"开箱照破", commands::OPEN_ENDLESS},  //梭哈在FF14的翻译是[照破]
    {"開箱梭哈", commands::OPEN_ENDLESS},  //繁體化
    {"開箱照破", commands::OPEN_ENDLESS},  //繁體化
    {"刷新箱子", commands::RELOAD},  //繁體化
    {"重載箱子", commands::RELOAD},  //繁體化

};
void msgDispatcher(const json& body)
{
    auto query = mirai::messageChainToArgs(body);
    if (query.empty()) return;

    auto cmd = query[0];
    if (commands_str.find(cmd) == commands_str.end()) return;

    auto m = mirai::parseMsgMetadata(body);

    if (!grp::groups[m.groupid].getFlag(grp::MASK_P | grp::MASK_CASE))
        return;

    json resp;
    switch (commands_str.at(cmd))
    {
    case commands::OPEN_1:
        resp = OPEN_1(m.groupid, m.qqid, query);
        break;
    case commands::OPEN_10:
        resp = OPEN_10(m.groupid, m.qqid, query);
        break;
    case commands::OPEN_SP2:
        resp = OPEN_SP2(m.groupid, m.qqid, query);
        break;
    case commands::OPEN_SP1:
        resp = OPEN_SP1(m.groupid, m.qqid, query);
        break;
    case commands::OPEN_ENDLESS:
        resp = OPEN_ENDLESS(m.groupid, m.qqid, query);
        break;
    case commands::RELOAD:
        resp = RELOAD(m.groupid, m.qqid, query);
        break;
    default: 
        break;
    }

    if (!resp.empty())
    {
        mirai::sendGroupMsg(m.groupid, resp);
    }
}

const case_detail& draw_case(double p)
{
    size_t idx = 0;

    if (p >= 0.0 && p < 1.0)
    {
        double totalp = 0;
        for (idx = 0; idx < CASE_TYPES.size() - 1; ++idx)
        {
            totalp += CASE_TYPES[idx].prob();
            if (p < totalp) break;
        }
        // idx = CASE_TYPES.size()-1 if not match any case
    }

    size_t detail_idx = randInt(0, CASE_POOL[idx].size() - 1);
    return CASE_POOL[idx][detail_idx];
}

void init(const char* case_list_yml)
{
    cfg_path = case_list_yml;
    loadCfg(case_list_yml);
}
}