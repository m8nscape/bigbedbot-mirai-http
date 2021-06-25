#include <sstream>
#include "eatwhat.h"

#include "data/group.h"
#include "utils/rand.h"
#include "mirai/api.h"
#include "mirai/msg.h"

namespace eatwhat {

std::string food::to_string(int64_t group)
{
    std::string qqname = "隔壁群友";
    if (group != 0 && offererType == food::QQ && offerer.qq != 0)
    {
        if (grp::groups.find(group) != grp::groups.end())
        {
            if (grp::groups[group].haveMember(offerer.qq))
                qqname = grp::groups[group].members[offerer.qq].nameCard;
        }
        else
        {
            auto minfo = mirai::getGroupMemberInfo(group, offerer.qq);
            qqname = minfo.nameCard;
        }
    }

    std::stringstream ss;
    ss << name;
    switch (offererType)
    {
    case food::NAME:  ss << " (" << offerer.name << "提供)"; break;
    case food::QQ:    ss << " (" << qqname << "提供)"; break;
    default: break;
    }

    //std::stringstream ssd;
    //ssd << offererType << ": " << name << " | " << offerer.name << " | " << offerer.qq;
    //addLogDebug("eat", ssd.str().c_str());
    return ss.str();
}

inline int addFood(food& f)
{
    const char query[] = "INSERT INTO food(name, adder, qq) VALUES (?,?,?)";
    int ret;
    switch (f.offererType)
    {
    case food::NAME:        ret = db.exec(query, { f.name, f.offerer.name, nullptr });  break;
    case food::QQ:          ret = db.exec(query, { f.name, nullptr, f.offerer.qq });    break;
    case food::ANONYMOUS:   ret = db.exec(query, { f.name, nullptr, nullptr });         break;
    default: break;
    }
    if (ret != SQLITE_OK)
    {
        addLog(LOG_WARNING, "eat", "Adding food %s failed (%s)", f.name.c_str(), db.errmsg());
        return 1;
    }

    //foodList.push_back(f);
    addLog(LOG_INFO, "eat", "Added food %s", f.name.c_str());
    return 0;
}

int getFood(food& f)
{
    const char query[] = "SELECT * FROM food ORDER BY RANDOM() limit 1";
    auto list = db.query(query, 4);
    if (list.empty()) return 1;
    else
    {
        auto &row = list[0];

        f.name = std::any_cast<std::string>(row[1]);
        if (row[2].has_value())
        {
            f.offererType = f.NAME;
            f.offerer.name = std::any_cast<std::string>(row[2]);
        }
        else if (row[3].has_value())
        {
            f.offererType = f.QQ;
            f.offerer.qq = std::any_cast<int64_t>(row[3]);
        }
        else
            f.offererType = f.ANONYMOUS;
    }

    return 0;

    //return foodList[randInt(0, foodList.size() - 1)];
}

int getFood10(food(&f)[10])
{
    const char query[] = "SELECT * FROM food ORDER BY RANDOM() limit 10";
    auto list = db.query(query, 4);
    size_t idx = 0;
    if (list.empty()) return 0;
    else
    {
        for (auto &row : list)
        {
            f[idx].name = std::any_cast<std::string>(row[1]);
            if (row[2].has_value())
            {
                f[idx].offererType = f[idx].NAME;
                f[idx].offerer.name = std::any_cast<std::string>(row[2]);
            }
            else if (row[3].has_value())
            {
                f[idx].offererType = f[idx].QQ;
                f[idx].offerer.qq = std::any_cast<int64_t>(row[3]);
            }
            else
                f[idx].offererType = f[idx].ANONYMOUS;

            idx++;
        }
    }

    return idx;

    //return foodList[randInt(0, foodList.size() - 1)];
}


int delFood(const std::string& name)
{
    const char query[] = "DELETE FROM food WHERE name=?";
    int ret = db.exec(query, { name });
    if (ret != SQLITE_OK)
    {
        addLog(LOG_ERROR, "eat", "Deleting food %s failed (%s)", name.c_str(), db.errmsg());
        return 1;
    }

    addLog(LOG_INFO, "eat", "Deleted food %s", name.c_str());
    return 0;
}

// idx starts from 1
inline int64_t getFoodIdx(const std::string& name = "")
{
    if (!name.empty())
    {
        const char query[] = "SELECT COUNT(*) FROM food WHERE name=?";
        auto list = db.query(query, 1, { name });
        return list.empty()? 0 : std::any_cast<int64_t>(list[0][0]);
    }
    else
    {
        const char query[] = "SELECT COUNT(*) FROM food";
        auto list = db.query(query, 1);
        return list.empty() ? 0 : std::any_cast<int64_t>(list[0][0]);
    }
}

inline int addDrink(drink& d)
{
    const char query[] = "INSERT INTO drink(name, qq, grp) VALUES (?,?,?)";
    int ret;
    ret = db.exec(query, { d.name, d.qq, d.group });
    if (ret != SQLITE_OK)
    {
        addLog(LOG_WARNING, "eat", "Adding drink %s failed (%s)", d.name.c_str(), db.errmsg());
        return 1;
    }

    //foodList.push_back(f);
    addLog(LOG_INFO, "eat", "Added drink %s", d.name.c_str());
    return 0;
}

int getDrink(drink& f)
{
    const char query[] = "SELECT * FROM drink ORDER BY RANDOM() limit 1";
    auto list = db.query(query, 4);
    if (list.empty()) return 1;
    else
    {
        auto &row = list[0];

        f.name = std::any_cast<std::string>(row[1]);
        f.qq = std::any_cast<int64_t>(row[2]);
        f.group = std::any_cast<int64_t>(row[3]);
    }

    return 0;
}

int delDrink(const std::string& name)
{
    const char query[] = "DELETE FROM drink WHERE name=?";
    int ret = db.exec(query, { name });
    if (ret != SQLITE_OK)
    {
        addLog(LOG_WARNING, "eat", "Deleting drink %s failed (%s)", name.c_str(), db.errmsg());
        return 1;
    }
    addLog(LOG_INFO, "eat", "Deleted drink %s", name.c_str());
    return 0;
}

// idx starts from 1
inline int64_t getDrinkIdx(const std::string& name = "")
{
    if (!name.empty())
    {
        const char query[] = "SELECT COUNT(*) FROM drink WHERE name=?";
        auto list = db.query(query, 1, { name });
        return list.empty() ? 0 : std::any_cast<int64_t>(list[0][0]);
    }
    else
    {
        const char query[] = "SELECT COUNT(*) FROM drink";
        auto list = db.query(query, 1);
        return list.empty() ? 0 : std::any_cast<int64_t>(list[0][0]);
    }
}

///////////////////////////////////////////////////////////////////////////////

const json ret_none = MSG_LINE("無");

json EATWHAT(::int64_t group, ::int64_t qq, std::vector<std::string> args)
{
    if (getFoodIdx() == 0) return ret_none;

    food f;
    getFood(f);
    json resp = mirai::MSG_TEMPLATE;
    json& r = resp["messageChain"];
    r.push_back(mirai::buildMessageAt(qq));
    r.push_back(mirai::buildMessagePlain("，你阔以选择"));
    r.push_back(mirai::buildMessagePlain(f.to_string(group)));
    return resp;
}

json DRINKWHAT(::int64_t group, ::int64_t qq, std::vector<std::string> args)
{
    if (getDrinkIdx() == 0) return ret_none;

    drink d;
    getDrink(d);
    json resp = mirai::MSG_TEMPLATE;
    json& r = resp["messageChain"];
    r.push_back(mirai::buildMessageAt(qq));
    r.push_back(mirai::buildMessagePlain("，你阔以选择"));
    r.push_back(mirai::buildMessagePlain(d.name));
    return resp;
}

json EATWHAT10(::int64_t group, ::int64_t qq, std::vector<std::string> args)
{
    if (getFoodIdx() == 0) return ret_none;

    json resp = mirai::MSG_TEMPLATE;
    json& r = resp["messageChain"];
    r.push_back(mirai::buildMessageAt(qq));
    r.push_back(mirai::buildMessagePlain("，你阔以选择：\n"));

    food f[10];
    int size = getFood10(f);
    for (int i = 0; i < size; ++i)
    {
        std::stringstream ss;
        ss << " - " << f[i].to_string(group) << "\n";
        r.push_back(mirai::buildMessagePlain(ss.str()));
    }
    r.push_back(mirai::buildMessagePlain("吃东西还十连，祝您撑死，哈"));

    return resp;
}

enum class CheckFoodResult
{
    OK,
    EMPTY,
    TOO_LONG,
    FILTERED
};

CheckFoodResult checkFood(const std::string& r)
{
    // TODO 我懒得写其他语种了，你们来写
    if (r.empty() || r == "空气" || r == "空氣") 
        return CheckFoodResult::EMPTY;

    if (r.length() > 30)
        return CheckFoodResult::TOO_LONG;
        
    //TODO filter

    return CheckFoodResult::OK;
}

std::string ADDFOOD(::int64_t group, ::int64_t qq, const std::string& r)
{
    switch (checkFood(r))
    {
    case CheckFoodResult::EMPTY:
        return "空气不能吃的";
    case CheckFoodResult::TOO_LONG:
        return "你就是飞天意面神教人？";
    case CheckFoodResult::FILTERED:
        return "你吃，我不吃";
    default:
        break;
    }

    // check repeat
    if (getFoodIdx(r))
        return r + "已经有了！！！";

    food f;
    f.name = r;
    f.offerer.qq = qq;
    f.offererType = f.QQ;
    if (addFood(f))
    {
        return "不准加";
    }
    std::stringstream ss;
    ss << "已添加" << f.to_string(group);
    return ss.str();
}

std::string DELFOOD(::int64_t group, ::int64_t qq, const std::string& r)
{
    if (!grp::checkPermission(group, qq, mirai::group_member_permission::MEMBER, true))
        return "你删个锤子？";

    if (r.empty()) return "空气不能吃的";

    int64_t count = getFoodIdx(r);
    if (count)
    {
        delFood(r);
    }

    std::stringstream ss;
    ss << "已删除" << count << "条" << r;
    return ss.str();
}

std::string ADDDRINK(::int64_t group, ::int64_t qq, const std::string& r)
{
    if (grp::checkPermission(group, qq, mirai::group_member_permission::OWNER, true))
        return "不是群主不给加";
        
    switch (checkFood(r))
    {
    case CheckFoodResult::EMPTY:
        return "空气不能喝的";
    case CheckFoodResult::TOO_LONG:
        return "我请你喝一根760mm汞柱";
    case CheckFoodResult::FILTERED:
        return "你喝，我不喝";
    default:
        break;
    }

    // check repeat
    if (getDrinkIdx(r))
        return r + "已经有了！！！";

    drink d;
    d.name = r;
    d.qq = qq;
    d.group = group;
    if (addDrink(d))
    {
        return "不准加";
    }
    std::stringstream ss;
    ss << "已添加" << d.name;
    return ss.str();
}

std::string DELDRINK(::int64_t group, ::int64_t qq, const std::string& r)
{
    if (grp::checkPermission(group, qq, mirai::group_member_permission::OWNER, true))
        return "你删个锤子？";

    if (r.empty()) return "空气不能喝的";
        
    int64_t count = getDrinkIdx(r);
    if (count)
    {
        delDrink(r);
    }

    std::stringstream ss;
    ss << "已删除" << count << "条" << r;
    return ss.str();
}

std::string MENU(::int64_t group, ::int64_t qq, std::vector<std::string> args)
{
    int64_t count = getFoodIdx();
    if (!count) return "无";

    return "MENU暂时不可用！";
    /*
    // defuault: last 9 entries
    size_t range_min = (count <= 9) ? 0 : (count - 9);
    size_t range_max = (count <= 9) ? (count - 1) : (range_min + 8);


    // arg[1] is range_mid
    if (args.size() > 1 && foodList.size() > 9) try
    {
        int tmp = std::stoi(args[1]) - 1 - 4;
        if (tmp < 0)
        {
            range_min = 0;
            range_max = 8;
        }
        else
        {
            range_min = tmp;
            range_max = range_min + 8;
        }

        if (range_max >= foodList.size()) {
            range_max = foodList.size() - 1;
            range_min = range_max - 8;
        }

    }
    catch (std::invalid_argument) {}

    if (range_min > range_max) return "";
    std::stringstream ret;
    for (size_t i = range_min; i <= range_max; ++i)
    {
        ret << i + 1 << ": " << foodList[i].to_string(group);
        if (i != range_max) ret << '\n';
    }

    return ret.str();
    */
}

enum class commands: size_t {
    EATWHAT,
    DRINKWHAT,
    EATWHAT10,
    ADDFOOD,
	DELFOOD,
	ADDDRINK,
	DELDRINK,
    MENU,
    DROPTABLE,
};

const std::map<std::string, commands> commands_str
{
    {"吃什么", commands::EATWHAT},
    {"吃什麼", commands::EATWHAT},   //繁體化
    {"喝什么", commands::DRINKWHAT},
    {"喝什麼", commands::DRINKWHAT},   //繁體化
    {"吃什么十连", commands::EATWHAT10},
    {"吃什麼十連", commands::EATWHAT10},   //繁體化
    {"加菜", commands::ADDFOOD},
    {"加菜", commands::ADDFOOD},   //繁體化
	{"减菜", commands::DELFOOD},
    {"減菜", commands::DELFOOD},   //繁體化
	{"删菜", commands::DELFOOD},
    {"刪菜", commands::DELFOOD},   //繁體化
	{"加饮料", commands::ADDDRINK},
    {"加飲料", commands::ADDDRINK},   //繁體化
	{"删饮料", commands::DELDRINK},
    {"刪飲料", commands::DELDRINK},   //繁體化
    {"菜单", commands::MENU},
    {"菜單", commands::MENU},   //繁體化
    //{"drop", commands::删库},
};

void msgDispatcher(const json& body)
{
    auto query = mirai::messageChainToArgs(body, 2);
    if (query.empty()) return;

    if (commands_str.find(query[0]) == commands_str.end()) return;
    auto c = commands_str.at(query[0]);
    
    auto m = mirai::parseMsgMetadata(body);

    if (m.groupid == 0)
    {
        return;
    }

    if (!grp::groups[m.groupid].getFlag(grp::Group::MASK_EAT))
        return;

    switch (c)
    {
    case commands::EATWHAT:
        mirai::sendMsgResp(m, EATWHAT(m.groupid, m.qqid, query));
        break;
    case commands::DRINKWHAT:
        mirai::sendMsgResp(m, DRINKWHAT(m.groupid, m.qqid, query));
        break;
    case commands::EATWHAT10:
        mirai::sendMsgResp(m, EATWHAT10(m.groupid, m.qqid, query));
        break;
    case commands::ADDFOOD:
        if (query.size() > 1)
            mirai::sendMsgRespStr(m, ADDFOOD(m.groupid, m.qqid, query[1]));
        break;
    case commands::DELFOOD:
        if (query.size() > 1)
            mirai::sendMsgRespStr(m, DELFOOD(m.groupid, m.qqid, query[1]));
        break;
    case commands::ADDDRINK:
        if (query.size() > 1)
            mirai::sendMsgRespStr(m, ADDDRINK(m.groupid, m.qqid, query[1]));
        break;
    case commands::DELDRINK:
        if (query.size() > 1)
            mirai::sendMsgRespStr(m, DELDRINK(m.groupid, m.qqid, query[1]));
        break;
    case commands::MENU:
        mirai::sendMsgRespStr(m, MENU(m.groupid, m.qqid, query));
        break;
    default: 
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////

void foodCreateTable()
{
    if (db.exec(
        "CREATE TABLE IF NOT EXISTS food( \
            id    INTEGER PRIMARY KEY AUTOINCREMENT, \
            name  TEXT    NOT NULL,       \
            adder TEXT,                   \
            qq    INTEGER                 \
         )") != SQLITE_OK)
        addLog(LOG_ERROR, "eat", db.errmsg());
}
/*
void foodLoadListFromDb()
{
    auto list = db.query("SELECT * FROM food", 4);
    for (auto& row : list)
    {
        food f;
        f.name = utf82gbk(std::any_cast<std::string>(row[1]));
        if (row[2].has_value())
        {
            f.offererType = f.NAME;
            f.offerer.name = utf82gbk(std::any_cast<std::string>(row[2]));
        }
        else if (row[3].has_value())
        {
            f.offererType = f.QQ;
            f.offerer.qq = std::any_cast<int64_t>(row[3]);
        }
        else
            f.offererType = f.ANONYMOUS;
        foodList.push_back(f);
    }
    char msg[128];
    sprintf(msg, "added %u foods", foodList.size());
    addLogDebug("eat", msg);
}
*/


void drinkCreateTable()
{
    if (db.exec(
        "CREATE TABLE IF NOT EXISTS drink( \
            id    INTEGER PRIMARY KEY AUTOINCREMENT, \
            name  TEXT    NOT NULL,        \
            qq    INTEGER,                 \
            grp   INTEGER                  \
         )") != SQLITE_OK)
        addLog(LOG_ERROR, "drink", db.errmsg());
}

}
