#include <sstream>
#include <list>
#include "eatwhat.h"

#include "data/group.h"
#include "utils/rand.h"
#include "mirai/api.h"
#include "mirai/msg.h"

namespace eatwhat {

const int64_t GROUP_ID_ALL = 0;
const int64_t GROUP_ID_OLD = 100;

std::string genSqlWhereGroupWithFlag(int64_t groupid)
{
    char buf[64] = {0};
    if (groupid == GROUP_ID_ALL || grp::Group::getFlag(groupid, grp::MASK_EAT_USE_UNIVERSE)) 
        ;//sprintf(buf, "");
    else if (grp::Group::getFlag(groupid, grp::MASK_EAT_USE_OLD))
        sprintf(buf, "WHERE grp=%ld OR grp=%ld", GROUP_ID_OLD, groupid);
    else
        sprintf(buf, "WHERE grp=%ld", groupid);
    return buf;
}
std::string genSqlWhereGroup(int64_t groupid)
{
    char buf[64] = {0};
    if (groupid == GROUP_ID_ALL) 
        ;//sprintf(buf, "");
    else
        sprintf(buf, "WHERE grp=%ld", groupid);
    return buf;
}
std::string genSqlWhereNameGroup(int64_t groupid)
{
    char buf[80] = {0};
    if (groupid == GROUP_ID_ALL) 
        sprintf(buf, "WHERE name=?");
    else
        sprintf(buf, "WHERE name=? AND grp=%ld", groupid);
    return buf;
}

std::string food::to_string(int64_t group)
{
    std::stringstream ss;
    ss << name;
    if (offererType == food::NAME)
    {
        ss << " (" << offerer.name << "提供)";
        return ss.str();
    }
    else if (offererType == food::ANONYMOUS)
    {
        return ss.str();
    }
    else if (offererType == food::QQ)
    {
        std::string qqname;
        if (offerer.qq != 0)
        {
            if (grp::groups.find(group) != grp::groups.end())
            {
                if (grp::groups[group].haveMember(offerer.qq))
                    qqname = grp::groups[group].members[offerer.qq].nameCard;
                else if (groupid == GROUP_ID_OLD)
                {
                    qqname = "隔壁群友";
                }
                else if (groupid != 0)
                {
                    std::stringstream ssqqname;
                    ssqqname << groupid << "群友";
                    qqname = ssqqname.str();
                }
            }
            else
            {
                auto minfo = mirai::getGroupMemberInfo(group, offerer.qq);
                qqname = minfo.nameCard;
            }
        }
        else
            qqname = "隔壁群友";

        ss << " (" << qqname << "提供)";
        return ss.str();
    }
    else
    {
        ss << "food offerer type error " << (int)offererType;
        throw std::out_of_range(ss.str());
    }
}

inline int addFood(food& f, int64_t groupid)
{
    // insert into db
    char query[80] = {0};
    sprintf(query, "INSERT INTO food(name, adder, qq, grp) VALUES (?,?,?,?)");
    int ret;
    switch (f.offererType)
    {
    case food::NAME:        ret = db.exec(query, { f.name, f.offerer.name, nullptr, groupid });  break;
    case food::QQ:          ret = db.exec(query, { f.name, nullptr, f.offerer.qq, groupid });    break;
    case food::ANONYMOUS:   ret = db.exec(query, { f.name, nullptr, nullptr, groupid });         break;
    default: break;
    }
    if (ret == SQLITE_OK)
    {
        addLog(LOG_INFO, "eat", "Added food %s", f.name.c_str());
        return 0;
    }
    else
    {
        addLog(LOG_WARNING, "eat", "Adding food %s failed (%s)", f.name.c_str(), db.errmsg());
        return 1;
    }
}

int getFood(food& f, int64_t groupid)
{
    // get from db
    char query[80] = {0};
    sprintf(query, "SELECT * FROM food %s ORDER BY RANDOM() limit 1", genSqlWhereGroupWithFlag(groupid).c_str());
    auto list = db.query(query, 5);
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
        f.groupid = std::any_cast<int64_t>(row[4]);
    }

    return 0;

    // get from cached list
    // if (foodList.empty()) return 1;
    // std::vector<food> tmpList{foodList.begin(), foodList.end()};
    // std::random_shuffle(tmpList.begin(), tmpList.end());
    // f = tmpList[0];
    // return 0;
}

int getFood10(food(&f)[10], int64_t groupid)
{
    // get from db
    size_t idx = 0;
    char query[80] = {0};
    sprintf(query, "SELECT * FROM food %s ORDER BY RANDOM() limit ?", genSqlWhereGroupWithFlag(groupid).c_str());
    auto list = db.query(query, 5, {10});
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
        f[idx].groupid = std::any_cast<int64_t>(row[4]);

        idx++;
    }

    return idx;

    // get from cached list
    // std::vector<food> tmpList{foodList.begin(), foodList.end()};
    // std::random_shuffle(tmpList.begin(), tmpList.end());
    // size_t upper_bound = std::min(10ul, tmpList.size());
    // for (size_t i = 0; i < upper_bound; ++i)
    // {
    //     f[i] = tmpList[i];
    // }
    // return upper_bound;
}


int delFood(const std::string& name, int64_t groupid)
{
    // del from db
    char query[80] = {0};
    sprintf(query, "DELETE FROM food %s", genSqlWhereNameGroup(groupid).c_str());
    int ret = db.exec(query, { name });
    if (ret == SQLITE_OK)
    {
        addLog(LOG_INFO, "eat", "Deleted food %s from group %ld", name.c_str(), groupid);
        return 0;
    }
    else
    {
        addLog(LOG_ERROR, "eat", "Deleting food %s failed (%s)", name.c_str(), db.errmsg());
        return 1;
    }

    // del from cached list
    // int count = 0;
    // for (auto it = foodList.begin(); it != foodList.end();)
    // {
    //     if (it->name == name)
    //     {
    //         ++count;
    //         it = foodList.erase(it);
    //     }
    //     else
    //     {
    //         ++it;
    //     }
    // }
}

// idx starts from 1
inline int64_t getFoodCount(const std::string& name = "", int64_t groupid = GROUP_ID_ALL)
{
    auto getCount = [](const std::string& name, int64_t groupid) -> int64_t
    {
        if (!name.empty())
        {
            char query[80] = {0};
            sprintf(query, "SELECT COUNT(*) FROM food %s", genSqlWhereNameGroup(groupid).c_str());
            auto list = db.query(query, 1, { name });
            return list.empty()? 0 : std::any_cast<int64_t>(list[0][0]);
        }
        else
        {
            char query[80] = {0};
            sprintf(query, "SELECT COUNT(*) FROM food %s", genSqlWhereGroup(groupid).c_str());
            auto list = db.query(query, 1);
            return list.empty()? 0 : std::any_cast<int64_t>(list[0][0]);
        }
    };
    //return (groupid == 0 ? 0 : getCount(name, 0)) + getCount(name, groupid);
    return getCount(name, groupid);
}

inline int64_t getFoodCount(int64_t groupid)
{
    return getFoodCount("", groupid);
}

inline int addDrink(drink& d, int64_t groupid)
{
    char query[80] = {0};
    sprintf(query, "INSERT INTO drink(name, qq, grp) VALUES (?,?,?)");
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

int getDrink(drink& f, int64_t groupid)
{
    char query[80] = {0};
    sprintf(query, "SELECT id,name,qq FROM drink %s ORDER BY RANDOM() limit 1", genSqlWhereGroupWithFlag(groupid).c_str());
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

int delDrink(const std::string& name, int64_t groupid)
{
    char query[80] = {0};
    sprintf(query, "DELETE FROM drink %s", genSqlWhereNameGroup(groupid).c_str());
    int ret = db.exec(query, { name });
    if (ret != SQLITE_OK)
    {
        addLog(LOG_WARNING, "eat", "Deleting drink %s failed (%s)", name.c_str(), db.errmsg());
        return 1;
    }
    addLog(LOG_INFO, "eat", "Deleted drink %s from group %ld", name.c_str(), groupid);
    return 0;
}

// idx starts from 1
inline int64_t getDrinkCount(const std::string& name = "", int64_t groupid = GROUP_ID_ALL)
{
    auto getCount = [](const std::string& name, int64_t groupid) -> int64_t
    {
        if (!name.empty())
        {
            char query[80] = {0};
            sprintf(query, "SELECT COUNT(*) FROM drink %s", genSqlWhereNameGroup(groupid).c_str());
            auto list = db.query(query, 1, { name });
            return list.empty()? 0 : std::any_cast<int64_t>(list[0][0]);
        }
        else
        {
            char query[80] = {0};
            sprintf(query, "SELECT COUNT(*) FROM drink %s", genSqlWhereGroup(groupid).c_str());
            auto list = db.query(query, 1);
            return list.empty()? 0 : std::any_cast<int64_t>(list[0][0]);
        }
    };
    //return (groupid == 0 ? 0 : getCount(name, 0)) + getCount(name, groupid);
    return getCount(name, groupid);
}
inline int64_t getDrinkCount(int64_t groupid)
{
    return getDrinkCount("", groupid);
}

bool hasFood(int64_t group)
{
    if (getFoodCount(group) == 0)
    {
        if (grp::Group::getFlag(group, grp::MASK_EAT_USE_UNIVERSE))
        {
            if (getFoodCount(GROUP_ID_ALL) == 0) return false;
        }
        else if (grp::Group::getFlag(group, grp::MASK_EAT_USE_OLD))
        {
            if (getFoodCount(GROUP_ID_OLD) == 0) return false;
        }
        else
        {
            return false;
        }
    }
    return true;
}
bool hasDrink(int64_t group)
{
    if (getDrinkCount(group) == 0)
    {
        if (grp::Group::getFlag(group, grp::MASK_EAT_USE_UNIVERSE))
        {
            if (getDrinkCount(GROUP_ID_ALL) == 0) return false;
        }
        else if (grp::Group::getFlag(group, grp::MASK_EAT_USE_OLD))
        {
            if (getDrinkCount(GROUP_ID_OLD) == 0) return false;
        }
        else
        {
            return false;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////

const json ret_none = MSG_LINE("無");

json EATWHAT(::int64_t group, ::int64_t qq, std::vector<std::string> args)
{
    if (!hasFood(group)) return ret_none;

    food f;
    getFood(f, group);
    grp::groups[group].sum_eatwhat += 1;
    json resp = mirai::MSG_TEMPLATE;
    json& r = resp["messageChain"];
    r.push_back(mirai::buildMessageAt(qq));
    r.push_back(mirai::buildMessagePlain("，你阔以选择"));
    r.push_back(mirai::buildMessagePlain(f.to_string(group)));
    return resp;
}

json DRINKWHAT(::int64_t group, ::int64_t qq, std::vector<std::string> args)
{
    if (!hasDrink(group)) return ret_none;

    drink d;
    getDrink(d, group);
    grp::groups[group].sum_eatwhat += 1;
    json resp = mirai::MSG_TEMPLATE;
    json& r = resp["messageChain"];
    r.push_back(mirai::buildMessageAt(qq));
    r.push_back(mirai::buildMessagePlain("，你阔以选择"));
    r.push_back(mirai::buildMessagePlain(d.name));
    return resp;
}

json EATWHAT10(::int64_t group, ::int64_t qq, std::vector<std::string> args)
{
    if (!hasFood(group)) return ret_none;

    json resp = mirai::MSG_TEMPLATE;
    json& r = resp["messageChain"];
    r.push_back(mirai::buildMessageAt(qq));
    r.push_back(mirai::buildMessagePlain("，你阔以选择：\n"));

    food f[10];
    int size = getFood10(f, group);
    grp::groups[group].sum_eatwhat += size;
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
    if ((grp::Group::getFlag(group, grp::MASK_EAT_USE_UNIVERSE) && getFoodCount(r, GROUP_ID_ALL) > 0) ||
        (grp::Group::getFlag(group, grp::MASK_EAT_USE_OLD) && getFoodCount(r, GROUP_ID_OLD) > 0) || 
        getFoodCount(r, group) > 0)
        return r + "已经有了！！！";

    food f;
    f.name = r;
    f.offerer.qq = qq;
    f.offererType = f.QQ;
    if (addFood(f, group))
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


    int64_t count = 0;
    if (grp::Group::getFlag(group, grp::MASK_EAT_USE_UNIVERSE) && (count = getFoodCount(r, GROUP_ID_ALL)) > 0)
    {
        delFood(r, GROUP_ID_ALL);
    }
    else if (grp::Group::getFlag(group, grp::MASK_EAT_USE_OLD) && (count = getFoodCount(r, GROUP_ID_OLD)) > 0)
    {
        delFood(r, GROUP_ID_OLD);
        delFood(r, group);
    }
    else if ((count = getFoodCount(r, group)) > 0)
    {
        delFood(r, group);
    }

    std::stringstream ss;
    if (count > 0)
    {
        ss << "已删除" << count << "条" << r;
    }
    else
    {
        ss << "没有";
    }

    return ss.str();
}

std::string ADDDRINK(::int64_t group, ::int64_t qq, const std::string& r)
{
    if (!grp::checkPermission(group, qq, mirai::group_member_permission::OWNER, true))
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
    if ((grp::Group::getFlag(group, grp::MASK_EAT_USE_UNIVERSE) && getDrinkCount(r, GROUP_ID_ALL) > 0) ||
        (grp::Group::getFlag(group, grp::MASK_EAT_USE_OLD) && getDrinkCount(r, GROUP_ID_OLD) > 0) || 
        getDrinkCount(r, group) > 0)
        return r + "已经有了！！！";

    drink d;
    d.name = r;
    d.qq = qq;
    d.group = group;
    if (addDrink(d, group))
    {
        return "不准加";
    }
    std::stringstream ss;
    ss << "已添加" << d.name;
    return ss.str();
}

std::string DELDRINK(::int64_t group, ::int64_t qq, const std::string& r)
{
    if (!grp::checkPermission(group, qq, mirai::group_member_permission::OWNER, true))
        return "你删个锤子？";

    if (r.empty()) return "空气不能喝的";
        
    int64_t count = 0;
    if (grp::Group::getFlag(group, grp::MASK_EAT_USE_UNIVERSE) && (count = getDrinkCount(r, GROUP_ID_ALL)) > 0)
    {
        delDrink(r, GROUP_ID_ALL);
    }
    else if (grp::Group::getFlag(group, grp::MASK_EAT_USE_OLD) && (count = getDrinkCount(r, GROUP_ID_OLD)) > 0)
    {
        delDrink(r, GROUP_ID_OLD);
        delDrink(r, group);
    }
    else if ((count = getDrinkCount(r, group)) > 0)
    {
        delDrink(r, group);
    }

    std::stringstream ss;
    if (count > 0)
    {
        ss << "已删除" << count << "条" << r;
    }
    else
    {
        ss << "没有";
    }
    return ss.str();
}

std::string MENU(::int64_t group, ::int64_t qq, std::vector<std::string> args)
{
#if NDEBUG
    const int64_t MENU_ENTRY_COUNT = 9;
#else
    const int64_t MENU_ENTRY_COUNT = 5;
#endif
    if (!hasFood(group)) return "无";

    int64_t count = 0;
    if (grp::Group::getFlag(group, grp::MASK_EAT_USE_UNIVERSE))
    {
        count = getFoodCount(GROUP_ID_ALL);
    }
    else
    {
        count = getFoodCount(group);

        if (grp::Group::getFlag(group, grp::MASK_EAT_USE_OLD))
            count += getFoodCount(GROUP_ID_OLD);
    }

    // defuault: last MENU_ENTRY_COUNT entries
    int64_t range_mid = count - 1;
    // arg[1] is range_mid
    if (args.size() > 1)
    {
        try
        {
            range_mid = std::max(0L, std::min(count, std::stol(args[1])) - 1);
        }
        catch (std::invalid_argument) {}
    }
    int64_t range_min = range_mid - MENU_ENTRY_COUNT / 2;
    int64_t range_max = range_mid + MENU_ENTRY_COUNT / 2;
    if (range_min < 0)
    {
        range_min = 0;
        range_max = std::min(MENU_ENTRY_COUNT - 1, count - 1);
    }
    else if (range_max >= count)
    {
        range_max = count - 1;
        range_min = std::max(0L, range_max - MENU_ENTRY_COUNT + 1);
    }

    char query[80] = {0};
    sprintf(query, "SELECT * FROM food %s LIMIT ? OFFSET ?", genSqlWhereGroupWithFlag(group).c_str());
    auto list = db.query(query, 4, {MENU_ENTRY_COUNT, range_min});

    std::stringstream ret;
    int i = range_min;
    for (auto &row : list) 
    {
        ret << i << ": " << std::any_cast<std::string>(row[1]);
        if (i++ < range_max) ret << '\n';
    }

    return ret.str();
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

    if (!grp::groups[m.groupid].getFlag(grp::MASK_EAT))
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

void init()
{
    foodCreateTable();
    foodLoadListFromDb();
    drinkCreateTable();
    drinkLoadListFromDb();
}

void foodCreateTable()
{
    char query[256] = {0};
    sprintf(query, 
        "CREATE TABLE IF NOT EXISTS food( "
            "id    INTEGER PRIMARY KEY AUTOINCREMENT, "
            "name  TEXT    NOT NULL, "
            "adder TEXT, "
            "qq    INTEGER, "
            "grp   INTEGER "
        ")");
    if (db.exec(query) != SQLITE_OK)
        addLog(LOG_ERROR, "eat", db.errmsg());

    sprintf(query, "SELECT name from sqlite_master where name='food' and sql like '%%grp%%'");
    auto list = db.query(query, 1);
    if (list.empty())
    {
        sprintf(query, "ALTER TABLE food ADD COLUMN grp INTEGER");
        if (db.exec(query) != SQLITE_OK)
            addLog(LOG_ERROR, "eat", db.errmsg());
        
        sprintf(query, "UPDATE food SET grp=? WHERE grp IS NULL");
        if (db.exec(query, {GROUP_ID_OLD}) != SQLITE_OK)
            addLog(LOG_ERROR, "eat", db.errmsg());
    }
}

void foodLoadListFromDb()
{
    char query[80] = {0};
    sprintf(query, "SELECT * FROM food");
    auto list = db.query(query, 4);
    for (auto& row : list)
    {
        food f;
        auto id = static_cast<unsigned>(std::any_cast<int64_t>(row[0]));
        //f.name = utf82gbk(std::any_cast<std::string>(row[1]));
        f.name = std::any_cast<std::string>(row[1]);
        if (row[2].has_value())
        {
            f.offererType = f.NAME;
            //f.offerer.name = utf82gbk(std::any_cast<std::string>(row[2]));
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
    addLogDebug("eat", "added %lu foods", list.size());
}


void drinkCreateTable()
{
    char query[256] = {0};
    sprintf(query, 
        "CREATE TABLE IF NOT EXISTS drink( "
            "id    INTEGER PRIMARY KEY AUTOINCREMENT, "
            "name  TEXT    NOT NULL, "
            "qq    INTEGER, "
            "grp   INTEGER "
        ")");
    if (db.exec(query) != SQLITE_OK)
        addLog(LOG_ERROR, "drink", db.errmsg());
}

void drinkLoadListFromDb()
{
    // char query[80] = {0};
    // sprintf(query, "SELECT * FROM %s", tableName.c_str());
    // auto list = db.query(query, 4);
    // drinkCount[groupid] = list.size();
    char query[80] = {0};
    sprintf(query, "SELECT COUNT(*) FROM drink");
    auto list = db.query(query, 1);
    int64_t drinkCountTotal = list.empty()? 0 : std::any_cast<int64_t>(list[0][0]);
    addLogDebug("eat", "added %ld drinks", drinkCountTotal);
}

}
