#include "event_case.h"

#include <iostream>
#include <sstream>
#include "cqp.h"
#include "appmain.h"

#include "utils/rand.h"
#include "utils/string_util.h"
#include "utils/time_util.h"

#include "data/user.h"
#include "data/group.h"

namespace event_case
{

case_pool::case_pool(const types_t& type_b, const levels_t& level_b, const std::vector<case_detail>& cases_b):
    types(type_b), levels(level_b)
{
    cases.resize(types.size());
    for (auto& c : cases)
        c.resize(levels.size());

    for (const auto& c : cases_b)
    {
        if (c.level < (int)types.size() && c.type < (int)types.size())
        {
            cases[c.type][c.level].second.push_back(c);
        }
    }

    // normalize level probability
    for (auto& type : cases)
    {
        unsigned level = 0;
        double total_p = 0.0;

        for (auto& [prop, case_c] : type)
        {
            if (!case_c.empty())
            {
                prop = levels[level].second;
                total_p += prop;
            }
            ++level;
        }

        for (auto& [prop, c] : type)
        {
            prop /= total_p;
        }
    }
}


case_detail case_pool::draw(int type)
{
    double p = randReal();
    if (type >= 0 && type < getTypeCount())
    {
        size_t idx = 0;

        if (p >= 0.0 && p <= 1.0)
        {
            double totalp = 0;
            for (const auto& [prob, list] : cases[type])
            {
                if (p <= totalp + prob) break;
                ++idx;
                totalp += prob;
            }
            // idx = CASE_POOL.size() if not match any case
        }

        size_t detail_idx = randInt(0, cases[type][idx].second.size() - 1);
        return cases[type][idx].second[detail_idx];
    }
    else
    {
        return {-1, -1, "非法箱子", -1};
    }
}

std::string case_pool::casePartName(const case_detail& c) const
{
    std::stringstream ss;
    //if (c.type >= 0 && c.type < getTypeCount()) ss << "[" << getType(c.type) << "] ";
    if (c.level >= 0 && c.level < getLevelCount()) ss << "<" << getLevel(c.level) << "> ";
    ss << c.name;
    ss << " (" << c.worth << "批)";

    std::string ret = ss.str();
    return ret;
}

std::string case_pool::caseFullName(const case_detail& c) const
{
    std::stringstream ss;
    if (c.type >= 0 && c.type < getTypeCount()) ss << "[" << getType(c.type) << "] ";
    if (c.level >= 0 && c.level < getLevelCount()) ss << "<" << getLevel(c.level) << "> ";
    ss << c.name;
    ss << " (" << c.worth << "批)";

    std::string ret = ss.str();
    return ret;
}

using user::plist;

case_pool pool_draw{{"测试", 1}, {"测试级", 1.0}, {{0, 0, "测试箱子", 1}}};
case_pool pool_drop{{"测试掉落", 1}, {"测试级", 1.0}, {{0, 0, "测试箱子", 1}}};

command msgDispatcher(const json& body)
{
    command c;
    auto query = mirai::messageChainToArgs(body);
    if (query.empty()) return c;

    auto cmd = query[0];
    if (commands_str.find(cmd) == commands_str.end()) return c;

    c.args = query;
    switch (c.c = commands_str[cmd])
    {
    case commands::开箱:
        c.func = [](::int64_t group, ::int64_t qq, std::vector<std::string> args, std::string raw) -> std::string
        {
            if (plist.find(qq) == plist.end()) return std::string(CQ_At(qq)) + "，你还没有开通菠菜";
			auto &p = plist[qq];

            std::stringstream ss;

            if (type < 0 || type >= pool_draw.getTypeCount())
            {
                ss << CQ_At(qq) << "，活动尚未开始，请下次再来";
                return ss.str();
            }

            auto [enough, stamina, rtime] = p.testStamina(1);
            if (!enough)
            {
                ss << CQ_At(qq) << "，你的体力不足，回满还需"
                    << rtime / (60 * 60) << "小时" << rtime / 60 % 60 << "分钟";
                return ss.str();
            }

            int cost = 0;
            if (type >= 0 && type < (int)pool_draw.getTypeCount() && p.getCurrency() < pool_draw.getTypeCost(type))
            {
                cost = pool_draw.getTypeCost(type);
                if (p.getCurrency() < cost)
                {
                    ss << CQ_At(qq) << "，你的余额不足，需要" << cost << "个批";
                    return ss.str();
                }
            }

            const case_detail& reward = pool_draw.draw(type);
            if (reward.worth > 300) ss << "歪哟，" << CQ_At(qq) << "发了，开出了";
            else ss << CQ_At(qq) << "，恭喜你开出了";
            ss << pool_draw.casePartName(reward);

			p.modifyStamina(1, true);
			p.modifyCurrency(+reward.worth - pool_draw.getTypeCost(type));

            // drop
            if (randReal() < 0.1)
            {
                /*
                ss << "，并获得1个箱子掉落";
                plist[qq].event_drop_count++;
                */
                ss << "，并获得1个箱子掉落，开出了";
                int type = randInt(0, pool_drop.getTypeCount() - 1);
                auto dcase = pool_drop.draw(type);
				p.modifyCurrency(+dcase.worth);
                ss << pool_drop.caseFullName(dcase);
            }

            //modifyBoxCount(qq, ++plist[qq].opened_box_count);
            //ss << "你还有" << stamina << "点体力，";

            return ss.str();
        };
        break;
    default: break;
    }
    return c;
}

void startEvent()
{
    if (type == -1)
    {
        type = randInt(0, pool_draw.getTypeCount() - 1);
        event_case_tm = getLocalTime(TIMEZONE_HR, TIMEZONE_MIN);
        std::stringstream ss;
        ss << "限时活动已开始，这次是<" << pool_draw.getType(type) << ">，每次收费" << pool_draw.getTypeCost(type) << "批，请群员踊跃参加";
        broadcastMsg(ss.str().c_str(), grp::Group::MASK_MONOPOLY);
    }
    else
    {
        addLog(LOG_WARNING, "event", "attempt to start event during event");
    }
}

void stopEvent()
{
    if (type != -1)
    {
        type = -1;
        auto event_case_time = time(nullptr);
        event_case_end_tm = getLocalTime(TIMEZONE_HR, TIMEZONE_MIN);

		broadcastMsg("限时活动已结束！", grp::Group::MASK_MONOPOLY);
    }
    else
    {
        addLog(LOG_WARNING, "event", "attempt to end event during normal time");
    }
}

case_pool loadCfg(const char* yaml)
{
	std::filesystem::path cfgPath(yaml);
	if (!std::filesystem::is_regular_file(cfgPath))
	{
		addLog(LOG_ERROR, "eventcase", "Case config file %s not found", std::filesystem::absolute(cfgPath));
		return -1;
	}
	addLog(LOG_INFO, "eventcase", "Loading case config from %s", std::filesystem::absolute(cfgPath));

	YAML::Node cfg = YAML::LoadFile(cfgPath);

    // type: {name, cost}
    case_pool::types_t types;
	for (const auto& it: cfg["types"])
	{
		if (it.size() >= 2)
			types.push_back({it[0].as<std::string>(), it[1].as<int>()});
	}

    // levels: {name, cost}
    case_pool::levels_t levels;
    double p = 0;
	for (const auto& it: cfg["levels"])
	{
		if (it.size() >= 2)
        {
            double pp = it[1].as<double>();
			levels.push_back({it[0].as<std::string>(), pp});
            p += pp;
        }
	}
    if (p > 1.0)
    {
		addLog(LOG_WARNING, "eventcase", "sum of level probabilities %lf exceeds 1.0", p);
    }

    // cases: {type, level, name, worth}
    const std::vector<case_detail> cases;
	for (const auto& it: cfg["list"])
	{
        if (it.size() >= 4)
        {
            size_t type = it[0].as<size_t>();
            size_t level = it[1].as<size_t>();
            if (type < types.size() && level < levels.size())
                cases.push_back({type, level, it[2].as<std::string>(), it[3].as<int>()});
        }
	}

	addLog(LOG_INFO, "eventcase", "Loaded %u cases", cases.size());
	return {types, levels, cases};
}

void init(const char* event_case_draw_yml, const char* event_case_drop_yml)
{
    pool_draw = loadCfg(event_case_draw_yml);
    pool_drop = loadCfg(event_case_drop_yml);
}

}