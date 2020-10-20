#include <sstream>
#include <filesystem>

#include "case.h"
#include "mirai/api.h"
#include "mirai/util.h"
#include "mirai/msg.h"
#include "data/user.h"
#include "utils/rand.h"
#include "logger.h"

#include "yaml-cpp/yaml.h"

namespace opencase
{
using user::plist;

std::vector<case_type> CASE_POOL
{
	{ "黑箱", /*-255, */  0.003 },
	//{ "彩箱", /*10000,*/  0.0006 },
	{ "黄箱", /*1000, */  0.0045 },
	{ "红箱", /*200,  */  0.01 },
	{ "粉箱", /*5,    */  0.03 },
	{ "紫箱", /*2,    */  0.17 },
};
case_type CASE_DEFAULT{ "蓝箱", /*1,*/ 1.0 };

case_detail::case_detail() : _type_idx(CASE_POOL.size()) {}
const case_type& case_detail::type() const { return (_type_idx < CASE_POOL.size() ? CASE_POOL[_type_idx] : CASE_DEFAULT); }

std::vector<std::vector<case_detail>> CASE_DETAILS;


json not_registered(int64_t qq)
{
	json resp = R"({ "messageChain": [] })"_json;
	resp["messageChain"].push_back(mirai::buildMessageAt(qq));
	resp["messageChain"].push_back(mirai::buildMessagePlain("，你还没有开通菠菜"));
	return resp;
}

json not_enough_currency(int64_t qq)
{
	json resp = R"({ "messageChain": [] })"_json;
	resp["messageChain"].push_back(mirai::buildMessageAt(qq));
	resp["messageChain"].push_back(mirai::buildMessagePlain("，你的余额不足"));
	return resp;
}

json not_enough_stamina(int64_t qq, time_t rtime)
{
	json resp = R"({ "messageChain": [] })"_json;
	resp["messageChain"].push_back(mirai::buildMessageAt(qq));
	std::stringstream ss;
	ss << "，你的体力不足，回满还需" << rtime / (60 * 60) << "小时" << rtime / 60 % 60 << "分钟";
	resp["messageChain"].push_back(mirai::buildMessagePlain(ss.str()));
	return resp;
}


json 开箱(::int64_t group, ::int64_t qq, std::vector<std::string> args)
{
	if (plist.find(qq) == plist.end()) 
		return not_registered(qq);

	auto &p = plist[qq];
	//CQ_setGroupBan(ac, group, qq, 60);
	//return "不准开";

	if (p.getKeyCount() >= 1)
	{
		p.modifyBoxCount(-1);
	}
	else if (p.getCurrency() >= FEE_PER_CASE)
	{
		auto[enough, stamina, rtime] = p.testStamina(1);
		if (!enough)
			return not_enough_stamina(qq, rtime);
		p.modifyCurrency(-FEE_PER_CASE);
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
	r.push_back(ss.str());

	p.modifyCurrency(reward.worth());
	p.modifyBoxCount(+1);
	//ss << "你还有" << stamina << "点体力，";

	return resp;
}

json 开箱10(::int64_t group, ::int64_t qq, std::vector<std::string> args)
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
		p.modifyStamina(-10, true);
	}
	else
		return not_enough_currency(qq);

	std::vector<int> case_counts(CASE_POOL.size() + 1, 0);
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
				((i == CASE_POOL.size()) ? CASE_DEFAULT.name() : CASE_POOL[i].name()) << "，";
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
				((i == CASE_POOL.size()) ? CASE_DEFAULT.name() : CASE_POOL[i].name()) << "，";
		}
	}
	ss << "一共" << rc << "个批";
	r.push_back(mirai::buildMessagePlain(ss.str()));

	p.modifyCurrency(+rc);
	p.modifyBoxCount(+10);

	return resp;
}

json 开红箱(::int64_t group, ::int64_t qq, std::vector<std::string> args)
{
	if (plist.find(qq) == plist.end())
		return not_registered(qq);

	auto &p = plist[qq];
	//CQ_setGroupBan(ac, group, qq, 60);
	//return "不准开";

	if (p.getCurrency() < COST_OPEN_RED)
		return not_enough_currency(qq);

	auto[enough, stamina, rtime] = p.testStamina(COST_OPEN_RED_STAMINA);

	if (!enough)
		return not_enough_stamina(qq, rtime);
	
	p.modifyCurrency(-COST_OPEN_RED);
	p.modifyStamina(-COST_OPEN_RED_STAMINA, true);

	json resp = R"({ "messageChain": [] })"_json;
	json& r = resp["messageChain"];
	std::stringstream ss;
	r.push_back(mirai::buildMessageAt(qq));
	ss << "消耗" << COST_OPEN_RED << "个批和" << COST_OPEN_RED_STAMINA << "点体力发动技能！\n";
	r.push_back(ss.str());
	ss.clear();

	std::vector<int> case_counts(CASE_POOL.size() + 1, 0);
	int count = 0;
	int cost = 0;
	int res = 0;
	int64_t pee = p.getCurrency();
	case_detail reward;
	do {
		++count;
		cost += FEE_PER_CASE;
		reward = draw_case(randReal());
		case_counts[reward.type_idx()]++;
		res += reward.worth();
		pee += reward.worth() - FEE_PER_CASE;

		if (reward.type_idx() == 2)
		{
			r.push_back(mirai::buildMessageAt(qq));
			ss << "开了" << count << "个箱子终于开出了" << reward.full() << " (" << reward.worth() << "批)，"
				<< "本次净收益" << pee - p.getCurrency() << "个批";
			r.push_back(mirai::buildMessagePlain(ss.str()));

			p.modifyCurrency(pee - p.getCurrency());
			p.modifyBoxCount(count);
			return resp;
		}
		else if (reward.type_idx() == 1)
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


	r.push_back(mirai::buildMessageAt(qq));
	ss << "破产了，开了" << count << "个箱子也没能开出红箱，"
		<< "本次净收益" << pee - p.getCurrency() << "个批";
	r.push_back(mirai::buildMessagePlain(ss.str()));

	p.modifyCurrency(pee - p.getCurrency());
	p.modifyBoxCount(count);
	return resp;

	//ss << "你还有" << stamina << "点体力，";
}

json 开黄箱(::int64_t group, ::int64_t qq, std::vector<std::string> args)
{
	if (plist.find(qq) == plist.end())
		return not_registered(qq);

	auto &p = plist[qq];
	//CQ_setGroupBan(ac, group, qq, 60);
	//return "不准开";

	if (p.getCurrency() < COST_OPEN_YELLOW)
		return not_enough_currency(qq);

	auto[enough, stamina, rtime] = p.testStamina(COST_OPEN_RED_STAMINA);

	if (!enough)
		return not_enough_stamina(qq, rtime);
	
	p.modifyCurrency(-COST_OPEN_YELLOW);
	p.modifyStamina(-user::MAX_STAMINA, true);

	json resp = R"({ "messageChain": [] })"_json;
	json& r = resp["messageChain"];
	std::stringstream ss;
	r.push_back(mirai::buildMessageAt(qq));
	ss << "消耗" << COST_OPEN_RED << "个批和" << user::MAX_STAMINA << "点体力发动技能！\n";
	r.push_back(ss.str());
	ss.clear();

	std::vector<int> case_counts(CASE_POOL.size() + 1, 0);
	int count = 0;
	int cost = 0;
	int res = 0;
	int64_t pee = p.getCurrency();
	case_detail reward;
	do {
		++count;
		cost += FEE_PER_CASE;
		reward = draw_case(randReal());
		case_counts[reward.type_idx()]++;
		res += reward.worth();
		pee += reward.worth() - FEE_PER_CASE;

		if (reward.type_idx() == 1)
		{
			r.push_back(mirai::buildMessageAt(qq));
			ss << "开了" << count << "个箱子终于开出了" << reward.full() << " (" << reward.worth() << "批)，"
				<< "本次净收益" << pee - p.getCurrency() << "个批";
			r.push_back(mirai::buildMessagePlain(ss.str()));

			p.modifyCurrency(pee - p.getCurrency());
			p.modifyBoxCount(count);
			return resp;
		}
	} while (pee >= FEE_PER_CASE);


	r.push_back(mirai::buildMessageAt(qq));
	ss << "破产了，开了" << count << "个箱子也没能开出红箱，"
		<< "本次净收益" << pee - p.getCurrency() << "个批";
	r.push_back(mirai::buildMessagePlain(ss.str()));

	p.modifyCurrency(pee - p.getCurrency());
	p.modifyBoxCount(count);
	return resp;

	//ss << "你还有" << stamina << "点体力，";
}

json 开箱endless(::int64_t group, ::int64_t qq, std::vector<std::string> args)
{
	json resp = R"({ "messageChain": [] })"_json;
	json& r = resp["messageChain"];
	r.push_back(mirai::buildMessagePlain("梭哈台被群主偷了，没得梭了"));
	return resp;
}
	/*
case commands::开箱endless:
	c.func = [](::int64_t group, ::int64_t qq, std::vector<std::string> args) -> std::string
	{
		if (plist.find(qq) == plist.end()) return std::string(CQ_At(qq)) + "，你还没有开通菠菜";
		//CQ_setGroupBan(ac, group, qq, 60);
		//return "不准开";

		if (plist[qq].currency < COST_OPEN_ENDLESS)
			return std::string(CQ_At(qq)) + "，你的余额不足";

		auto [enough, stamina, rtime] = updateStamina(qq, COST_OPEN_ENDLESS_STAMINA);

		std::stringstream ss;
		if (!enough) ss << CQ_At(qq) << "，你的体力不足，回满还需"
			<< rtime / (60 * 60) << "小时" << rtime / 60 % 60 << "分钟";
		else
		{
			plist[qq].currency -= COST_OPEN_ENDLESS;
			long long total_cases = plist[qq].currency / FEE_PER_CASE;
			plist[qq].currency %= FEE_PER_CASE;
			long long extra_cost = 0.1 * total_cases * FEE_PER_CASE;
			total_cases -= long long(std::floor(total_cases * 0.1));
			ss << getCard(group, qq) << "消耗" << COST_OPEN_ENDLESS + extra_cost << "个批和" << COST_OPEN_ENDLESS_STAMINA << "点体力发动技能！\n";
			std::vector<int> case_counts(CASE_POOL.size() + 1, 0);
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
				<< "本次净收益" << res - cost - COST_OPEN_ENDLESS - extra_cost << "个批";
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

const std::map<std::string, commands> commands_str
{
	{"开箱", commands::开箱},
	{"開箱", commands::开箱},  //繁體化
	{"开箱十连", commands::开箱10},
	{"開箱十連", commands::开箱10},  //繁體化
	{"开黄箱", commands::开黄箱},
	{"開黃箱", commands::开黄箱},  //繁體化
	{"开红箱", commands::开红箱},
	{"開紅箱", commands::开红箱},  //繁體化
	{"开箱梭哈", commands::开箱endless},
	{"开箱照破", commands::开箱endless},  //梭哈在FF14的翻译是[照破]
	{"開箱梭哈", commands::开箱endless},  //繁體化
	{"開箱照破", commands::开箱endless},  //繁體化

};
void msgDispatcher(const json& body)
{
	auto query = mirai::messageChainToArgs(body);
	if (query.empty()) return;

	auto cmd = query[0];
	if (commands_str.find(cmd) == commands_str.end()) return;

	int msgid = 0;
	time_t timestamp = 0;
	int64_t qq = 0;
	int64_t group = 0;
	mirai::parseMsgMeta(body, msgid, timestamp, qq, group);

	json resp;
	switch (commands_str.at(cmd))
	{
	case commands::开箱:
		resp = 开箱(group, qq, query);
		break;
	case commands::开箱10:
		resp = 开箱10(group, qq, query);
		break;
	case commands::开红箱:
		resp = 开红箱(group, qq, query);
		break;
	case commands::开黄箱:
		resp = 开黄箱(group, qq, query);
		break;
	case commands::开箱endless:
		resp = 开箱endless(group, qq, query);
		break;
	default: 
		break;
	}

	if (!resp.empty())
	{
		mirai::sendGroupMsg(group, resp);
	}
}

const case_detail& draw_case(double p)
{
	size_t idx = 0;

	if (p >= 0.0 && p <= 1.0)
	{
		double totalp = 0;
		for (const auto& c : CASE_POOL)
		{
			if (p <= totalp + c.prob()) break;
			++idx;
			totalp += c.prob();
		}
		// idx = CASE_POOL.size() if not match any case
	}

	size_t detail_idx = randInt(0, CASE_DETAILS[idx].size() - 1);
	return CASE_DETAILS[idx][detail_idx];
}

int loadCases(const char* yaml)
{
	std::filesystem::path cfgPath(yaml);
	if (!std::filesystem::is_regular_file(cfgPath))
	{
		addLog(LOG_ERROR, "case", "Case config file %s not found", std::filesystem::absolute(cfgPath).c_str());
		return -1;
	}
	addLog(LOG_INFO, "user", "Loading case config from %s", std::filesystem::absolute(cfgPath).c_str());

	YAML::Node cfg = YAML::LoadFile(cfgPath);

	CASE_POOL.clear();
    double p = 0;
	for (const auto& it: cfg["levels"])
	{
		if (it.size() >= 2)
		{
            double pp = it[1].as<double>();
			CASE_POOL.push_back({it[0].as<std::string>(), pp});
			p += pp;
		}
		else
		{
			CASE_POOL.push_back({it[0].as<std::string>(), 1.0 - p});
		}
	}
	if (p > 1.0)
	{
		addLog(LOG_WARNING, "case", "sum of level probabilities %lf exceeds 1.0", p);
	}

	CASE_DETAILS.clear();
	CASE_DETAILS.resize(CASE_POOL.size() + 1);
	size_t case_count = 0;
	for (const auto& it: cfg["list"])
	{
		size_t level = it[0].as<size_t>();
		if (level < CASE_DETAILS.size())
			CASE_DETAILS[level].push_back({level, it[1].as<std::string>(), it[2].as<int>()});
		++case_count;
	}

	addLog(LOG_INFO, "case", "Loaded %u cases", case_count);
	return 0;
}

void init(const char* case_list_yml)
{
	loadCases(case_list_yml);
}
}