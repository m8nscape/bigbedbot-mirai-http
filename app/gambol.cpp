#include "gambol.h"
#include <sstream>
#include <set>
#include <thread>
#include <regex>
#include "utils/logger.h"
#include "utils/rand.h"
#include "data/user.h"
#include "data/group.h"

using namespace std::string_literals;
std::string str_put_fail = "你会不会⬇注？";
std::string str_nobody = "無人⬇注，取消本局";

namespace gambol
{
using user::plist;

typedef std::function<void(::int64_t, ::int64_t, std::vector<std::string>&, const char*)> callback;
struct command
{
    commands c = (commands)0;
    std::vector<std::string> args;
    callback func = nullptr;
};

std::map<int64_t, gameData> groupMap;

std::string CQ_At(int64_t qq) {return std::to_string(qq);}
std::string getCard(int64_t group, int64_t qq) {return std::to_string(qq);}

///////////////////////////////////////////////////////////////////////////////

using nlohmann::json;

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

///////////////////////////////////////////////////////////////////////////////

void msgDispatcher(const json& body)
{
    command c;
    std::string query = mirai::messageChainToStr(body);
    if (query.empty()) return;
    if (query.length() > 32) return;

    auto mm = mirai::parseMsgMetadata(body);
    if (!grp::groups[mm.groupid].getFlag(grp::Group::MASK_GAMBOL))
        return;

    // cmd:
    static std::regex reFlipcoinStart = std::regex(R"((开|開)始翻批( +(正|反) +(\d+))?)");
    static std::regex reFlipcoinChoose = std::regex(R"((翻(批)?)? *(正|反) +(\d+))");
    static std::regex reRouletteStart = std::regex(R"((开|開)始(摇号|搖號)( +(.+?) +(\d+))?)");
    static std::regex reRouletteChoose = std::regex(R"((摇|搖(号|號)?) *(.+?) +(\d+))");
    std::smatch m;
    if (std::regex_match(query, m, reFlipcoinStart))
    {
        c.args = {"开始翻批", m[3], m[4]};
        c.c = commands::FLIPCOIN_START;
    }
    else if (std::regex_match(query, m, reFlipcoinChoose))
    {
        c.args = {m[3], m[4]};
        if (m[3] == "正")
            c.c = commands::FLIPCOIN_FRONT;
        else
            c.c = commands::FLIPCOIN_BACK;
    }
    else if (std::regex_match(query, m, reRouletteStart))
    {
        c.args = {"开始摇号", m[4], m[5]};
        c.c = commands::ROULETTE_START;
    }
    else if (std::regex_match(query, m, reRouletteChoose))
    {
        c.args = {"摇号", m[3], m[4]};
        c.c = commands::ROULETTE_CHOOSE;
    }
    else return;

    switch (c.c)
    {
    case commands::FLIPCOIN_START:
        c.func = [](::int64_t group, ::int64_t qq, std::vector<std::string> args, const char* raw)
        {
            if (user::plist.find(qq) == user::plist.end()) 
            {
                mirai::sendGroupMsg(group, not_registered(qq));
                return;
            }

            flipcoin::roundStart(group);
            if (args.size() > 2)
            {
                int64_t p = 0;
                try {
                    p = std::stoll(args[2]);
                }
                catch (std::exception&) {}
                if (p < 0) p = 0;

                if (p <= 0) return;
                else if (args[1] == "正") flipcoin::put(group, qq, { p, 0 });
                else if (args[1] == "反") flipcoin::put(group, qq, { 0, p });
            }
        };
        break;
    case commands::FLIPCOIN_FRONT:
        c.func = [](::int64_t group, ::int64_t qq, std::vector<std::string> args, const char* raw)
        {
            if (user::plist.find(qq) == user::plist.end()) 
            {
                mirai::sendGroupMsg(group, not_registered(qq));
                return;
            }

            if (args.size() > 1)
            {
                int64_t p = 0;
                try {
                    p = std::stoll(args[1]);
                }
                catch (std::exception&) {}
                if (p < 0) p = 0;

                if (p <= 0) return;
                flipcoin::put(group, qq, { p, 0 });
            }
        };
        break;
    case commands::FLIPCOIN_BACK:
        c.func = [](::int64_t group, ::int64_t qq, std::vector<std::string> args, const char* raw)
        {
            if (user::plist.find(qq) == user::plist.end()) 
            {
                mirai::sendGroupMsg(group, not_registered(qq));
                return;
            }

            if (args.size() > 1)
            {
                int64_t p = 0;
                try {
                    p = std::stoll(args[1]);
                }
                catch (std::exception&) {}
                if (p < 0) p = 0;

                if (p <= 0) return;
                flipcoin::put(group, qq, { 0, p });
            }
        };
        break;


    case commands::ROULETTE_START:
        c.func = [](::int64_t group, ::int64_t qq, std::vector<std::string> args, const char* raw)
        {
            if (user::plist.find(qq) == user::plist.end()) 
            {
                mirai::sendGroupMsg(group, not_registered(qq));
                return;
            }

            roulette::roundStart(group);
            if (args.size() > 2)
            {
                int64_t p = 0;
                try {
                    p = std::stoll(args[2]);
                }
                catch (std::exception&) {}
                if (p < 0) p = 0;

                std::string subc = args[1];
                if (p <= 0 || roulette::gridTokens.find(subc) == roulette::gridTokens.end()) return;
                else roulette::put(group, qq, roulette::gridTokens.at(subc), p);
            }
        };
        break;

    case commands::ROULETTE_CHOOSE:
        c.func = [](::int64_t group, ::int64_t qq, std::vector<std::string> args, const char* raw)
        {
            if (user::plist.find(qq) == user::plist.end()) 
            {
                mirai::sendGroupMsg(group, not_registered(qq));
                return;
            }

            if (args.size() > 2)
            {
                int64_t p = 0;
                try {
                    p = std::stoll(args[2]);
                }
                catch (std::exception&) {}
                if (p < 0) p = 0;

                std::string subc = args[1];
                if (p <= 0 || roulette::gridTokens.find(subc) == roulette::gridTokens.end()) return;
                else roulette::put(group, qq, roulette::gridTokens.at(subc), p);
            }
        };
        break;

    default:
        break;
    }

    c.func(mm.groupid, mm.qqid, c.args, query.c_str());
}

///////////////////////////////////////////////////////////////////////////////
// flipcoin

namespace flipcoin
{

void roundStart(int64_t group)
{
    if (groupMap[group].flipcoin_running)
    {
        mirai::sendGroupMsgStr(group, "There is already a game running at this group.");
        return;
    }

    mirai::sendGroupMsgStr(group, "新一轮的翻批开始了，欢迎水友们踊跃参加");
	groupMap[group].flipcoin = {};
	groupMap[group].flipcoin_running = true;
	groupMap[group].flipcoin.startTime = time(nullptr);
    std::thread([=]() {
        using namespace std::chrono_literals;

        // 40s
        std::this_thread::sleep_for(20s);
        roundAnnounce(group);

        // 20s
        std::this_thread::sleep_for(20s);
        roundAnnounce(group);

        // 10s
        std::this_thread::sleep_for(10s);
        roundAnnounce(group);

        // 5s
        std::this_thread::sleep_for(5s);
        //flipcoin::roundAnnounce(group);

        // end
        std::this_thread::sleep_for(5s);
        roundEnd(group);

        }).detach();
}

void roundAnnounce(int64_t group)
{
    if (!groupMap[group].flipcoin_running)
    {
        //mirai::sendGroupMsgStr(group, "No flipcoin round is running at this group.");
        return;
    }

    std::stringstream ss;
    const auto& r = groupMap[group].flipcoin;
    ss << "本轮翻批时间还剩" << r.startTime + 60 - time(nullptr) << "秒\n";

    ss << "总计" << r.total << "个批，正面" << r.front << "个，反面" << r.back << "个";
    if (r.total > 0)
    {
        ss << "\n";
        ss << "正面：";
        for (const auto& [qq, stat] : r.pee_per_player)
        {
            if (stat.front)
                ss << qq << "(" << stat.front << "个批) "
                << "(" << 50.0 * stat.front / r.front << "%)  ";
        }
        ss << "\n";
        ss << "反面：";
        for (const auto& [qq, stat] : r.pee_per_player)
        {
            if (stat.back)
                ss << qq << "(" << stat.back << "个批) "
                << "(" << 50.0 * stat.back / r.back << "%)  ";
        }
    }
    mirai::sendGroupMsgStr(group, ss.str());
}

void roundEnd(int64_t group)
{
    if (!groupMap[group].flipcoin_running)
    {
        //mirai::sendGroupMsgStr(group, "No flipcoin round is running at this group.");
        return;
    }

    const auto& r = groupMap[group].flipcoin;
    if (r.front == 0 || r.back == 0)
    {
        mirai::sendGroupMsgStr(group, str_nobody);
        roundCancel(group);
        return;
    }

    bool front = randInt(0, 1);
    double per = randReal();

    int64_t deno = front ? r.front : r.back;
    double totalper = 0;
    int64_t winner;

    if (deno != 0)
    {
        for (const auto& [qq, bet] : r.pee_per_player)
        {
            totalper += 1.0 * (front ? bet.front : bet.back) / deno;
            if (totalper >= per)
            {
                winner = qq;
                break;
            }
        }
    }

    std::stringstream ss;
    ss << "本轮翻批结束，结果为" << (front ? "正面" : "反面") << "，";
    if (deno != 0)
    {
        auto& w = r.pee_per_player.at(winner);
        int64_t bet = (front ? w.front : w.back);
        ss << CQ_At(winner) << "以" << 50.0 * bet / deno
            << "%的概率赢得巨款" << r.total << "个批";
        int64_t tmpTotal = r.total;
        while (tmpTotal > 0)
        {
            ss << "！";
            tmpTotal /= 10;
        }
    }
    else
        ss << r.total << "个批么得咯";
    mirai::sendGroupMsgStr(group, ss.str());

	user::plist[winner].modifyCurrency(+r.total);
    grp::groups[group].sum_earned += r.total;
    groupMap[group].flipcoin_running = false;
}

void roundCancel(int64_t group)
{
    if (!groupMap[group].flipcoin_running)
        return;

    for (const auto& [qq, stat] : groupMap[group].flipcoin.pee_per_player)
    {
        if (stat.front) 
        {
            user::plist[qq].modifyCurrency(+stat.front);
            grp::groups[group].sum_spent -= stat.front;
        }
		if (stat.back)
        {
            user::plist[qq].modifyCurrency(+stat.back);
            grp::groups[group].sum_spent -= stat.back;
        }
    }

    groupMap[group].flipcoin_running = false;
    mirai::sendGroupMsgStr(group, "批不翻了，返还所有批");
}

void roundCancelAll()
{
    for (auto& [group, data]: groupMap)
    {
        if (data.flipcoin_running)
            roundCancel(group);
    }
}

void put(int64_t group, int64_t qq, bet bet)
{
    if (!groupMap[group].flipcoin_running)
    {
        mirai::sendGroupMsgStr(group, "本群么得开盘啊");
        return;
    }

    if (bet.front + bet.back <= 0)
    {
        mirai::sendGroupMsgStr(group, "你就是负批怪？");
        return;
    }

    if (user::plist[qq].getCurrency() < bet.front + bet.back)
    {
        mirai::sendGroupMsg(group, not_enough_currency(qq));
        return;
    }

    auto& round = groupMap[group].flipcoin;
    auto& player = round.pee_per_player[qq];
    player.front += bet.front;
    player.back += bet.back;

    round.front += bet.front;
    round.back += bet.back;
    round.total += bet.front + bet.back;

    int64_t spent = bet.front + bet.back;
	user::plist[qq].modifyCurrency(-spent);
    grp::groups[group].sum_spent += spent;

    std::stringstream ss;
    if (bet.front)
    {
        if (player.front == bet.front)
            ss << getCard(group, qq) << "成功⬇" << "正面" << bet.front << "个批";
        else
            ss << getCard(group, qq) << "成功⬇" << "正面" << "到" << player.front << "个批";
    }
    else if (bet.back)
    {
        if (player.back == bet.back)
            ss << getCard(group, qq) << "成功⬇" << "反面" << bet.back << "个批";
        else
            ss << getCard(group, qq) << "成功⬇" << "反面" << "到" << player.back << "个批";
    }

    mirai::sendGroupMsgStr(group, ss.str());
}
}


namespace roulette
{

void roundStart(int64_t group)
{
    if (groupMap[group].roulette_running)
    {
        mirai::sendGroupMsgStr(group, "There is already a game running at this group.");
        return;
    }

    mirai::sendGroupMsgStr(group, "新一轮的摇号开始了，欢迎水友们踊跃参加");
	groupMap[group].roulette = {};
    groupMap[group].roulette_running = true;
    groupMap[group].roulette.startTime = time(nullptr);
    std::thread([=]() {
        using namespace std::chrono_literals;

        // 40s
        std::this_thread::sleep_for(20s);
        roundAnnounce(group);

        // 20s
        std::this_thread::sleep_for(20s);
        roundAnnounce(group);

        // 10s
        std::this_thread::sleep_for(10s);
        roundAnnounce(group);

        // 5s
        std::this_thread::sleep_for(5s);
        //roulette::roundAnnounce(group);

        // end
        std::this_thread::sleep_for(5s);
        roundEnd(group);

        }).detach();
}

void roundAnnounce(int64_t group)
{
    if (!groupMap[group].roulette_running)
    {
        //mirai::sendGroupMsgStr(group, "No roulette round is running at this group.");
        return;
    }

    std::stringstream ss;
    const auto& r = groupMap[group].roulette;
    ss << "本轮摇号时间还剩" << r.startTime + 60 - time(nullptr) << "秒\n";

    ss << "总计" << r.total << "个批";
    if (r.total > 0)
    {
        /*
        for (size_t i = 0; i < GRID_COUNT; ++i)
            if (r.amount[i])
                ss << "，" << gridName[i] << ": " << r.amount[i];
                */

        for (size_t i = 0; i < GRID_COUNT; ++i)
        {
            if (!r.amount[i]) continue;
            ss << "\n";
            ss << gridName[i] << ": ";
            for (const auto& [qq, stat] : r.pee_per_player)
                if (stat.amount[i])
                    ss << getCard(group, qq) << "("<< stat.amount[i] << "个批) ";
        }
    }
    mirai::sendGroupMsgStr(group, ss.str());
}

void roundEnd(int64_t group)
{
    if (!groupMap[group].roulette_running)
    {
        //mirai::sendGroupMsgStr(group, "No roulette round is running at this group.");
        return;
    }

    const auto& r = groupMap[group].roulette;
    if (r.total == 0)
    {
        mirai::sendGroupMsgStr(group, str_nobody);
        roundCancel(group);
        return;
    }

    int result = randInt(0, 36);
    std::set<unsigned> red_idx{ 1, 3, 5, 7, 9, 12, 14, 16, 18, 19, 21, 23, 25, 27, 30, 32, 34, 36 };
    std::set<unsigned> blk_idx{ 2, 4, 6, 8, 10, 11, 13, 15, 17, 20, 22, 24, 26, 28, 29, 31, 33, 35 };
    bool b_red = red_idx.find(result) != red_idx.end();
    bool b_black = blk_idx.find(result) != blk_idx.end();
    bool b_odd = result % 2 != 0;
    bool b_even = !b_odd && result != 0;
    bool b_1st = result >= 1 && result <= 12;
    bool b_2nd = result >= 13 && result <= 24;
    bool b_3rd = result >= 25 && result <= 36;

    std::map<int64_t, int64_t> reward;
    for (const auto& [qq, bet] : r.pee_per_player)
    {
        int64_t reward_tmp = 0;
        if (result == 0 && bet.amount[0]) reward_tmp += bet.amount[0] * 50;
        if (result != 0 && bet.amount[result]) reward_tmp += bet.amount[result] * 36;
        if (b_black && bet.amount[Cblack]) reward_tmp += bet.amount[Cblack] * 2;
        if (b_red && bet.amount[Cred]) reward_tmp += bet.amount[Cred] * 2;
        if (b_odd && bet.amount[Aodd]) reward_tmp += bet.amount[Aodd] * 2;
        if (b_even && bet.amount[Aeven]) reward_tmp += bet.amount[Aeven] * 2;
        if (b_1st && bet.amount[P1st]) reward_tmp += bet.amount[P1st] * 3;
        if (b_2nd && bet.amount[P2nd]) reward_tmp += bet.amount[P2nd] * 3;
        if (b_3rd && bet.amount[P3rd]) reward_tmp += bet.amount[P3rd] * 3;
        if (reward_tmp)
        {
            addLogDebug("duel", ("reward: "s + std::to_string(qq) + " " + std::to_string(reward_tmp)).c_str());
			plist[qq].modifyCurrency(reward_tmp);
            grp::groups[group].sum_earned += reward_tmp;
            reward[qq] = reward_tmp;
        }
    }

    std::stringstream ss;
    ss << "本轮摇号结束，结果为："
        << "[" << result << "]";
    if (result != 0)
        ss << (b_black ? " <黑>" : " <红>")
            << (b_odd ? " <单>" : " <双>")
            << (b_1st ? " <1st>" : "")
            << (b_2nd ? " <2nd>" : "")
            << (b_3rd ? " <3rd>" : "");
    ss << "\n";
    if (!reward.empty())
    {
        for (auto& [qq, amount] : reward)
            ss << CQ_At(qq) << "获得" << amount << "个批";
    }
    else
    {
        ss << "批么得咯";
    }
    mirai::sendGroupMsgStr(group, ss.str());
    groupMap[group].roulette_running = false;
}

void roundCancel(int64_t group)
{
    if (!groupMap[group].roulette_running)
        return;

    for (const auto& [qq, stat] : groupMap[group].roulette.pee_per_player)
    {
		int64_t total = 0;
        for (const auto& a : stat.amount)
            total += a;
		plist[qq].modifyCurrency(+total);
        grp::groups[group].sum_spent -= total;
    }

    groupMap[group].roulette_running = false;
    mirai::sendGroupMsgStr(group, "号不摇了，返还所有批");
}

void roundCancelAll()
{
    for (auto& [group, data]: groupMap)
    {
        if (data.roulette_running)
            roundCancel(group);
    }
}

void put(int64_t group, int64_t qq, grid g, int64_t amount)
{
    if (!groupMap[group].roulette_running)
    {
        mirai::sendGroupMsgStr(group, "本群么得开盘啊");
        return;
    }

    if (amount < 0)
    {
        mirai::sendGroupMsgStr(group, "你就是负批怪？");
        return;
    }

    if (user::plist[qq].getCurrency() < amount)
    {
        mirai::sendGroupMsg(group, not_enough_currency(qq));
        return;
    }

    auto& round = groupMap[group].roulette;
    auto& player = round.pee_per_player[qq];
    player.amount[g] += amount;

    round.amount[g] += amount;
    round.total += amount;

	plist[qq].modifyCurrency(-amount);
    grp::groups[group].sum_spent += amount;

    std::stringstream ss;
    if (player.amount[g] == amount)
        ss << getCard(group, qq) << "成功⬇[" << gridName[g] << "]" << amount << "个批";
    else
        ss << getCard(group, qq) << "成功⬇[" << gridName[g] << "]到" << player.amount[g] << "个批";

    mirai::sendGroupMsgStr(group, ss.str());
}
}

}

