#pragma once
#include <vector>
#include <map>
#include <string>
#include <functional>
#include <iostream>

#include "common/dbconn.h"

namespace eatwhat {

inline SQLite db("eat.db", "eat");

enum class commands: size_t {
    吃什么,
    喝什么,
    玩什么,
    吃什么十连,
    加菜,
	删菜,
	加饮料,
	删饮料,
    菜单,
    删库,
};

inline std::map<std::string, commands> commands_str
{
    {"吃什么", commands::吃什么},
    {"吃什麼", commands::吃什么},   //繁體化
    {"喝什么", commands::喝什么},
    {"喝什麼", commands::喝什么},   //繁體化
    {"玩什么", commands::玩什么},
    {"玩什麼", commands::玩什么},   //繁體化
    {"吃什么十连", commands::吃什么十连},
    {"吃什麼十連", commands::吃什么十连},   //繁體化
    {"加菜", commands::加菜},
    {"加菜", commands::加菜},   //繁體化
	{"减菜", commands::删菜},
    {"減菜", commands::删菜},   //繁體化
	{"删菜", commands::删菜},
    {"刪菜", commands::删菜},   //繁體化
	{"加饮料", commands::加饮料},
    {"加飲料", commands::加饮料},   //繁體化
	{"删饮料", commands::删饮料},
    {"刪飲料", commands::删饮料},   //繁體化
    {"菜单", commands::菜单},
    {"菜單", commands::菜单},   //繁體化
    //{"drop", commands::删库},
};

typedef std::function<std::string(::int64_t, ::int64_t, std::vector<std::string>&, const char*)> callback;
//typedef std::string(*callback)(::int64_t, ::int64_t, std::vector<std::string>);
struct command
{
    commands c = (commands)0;
    std::vector<std::string> args;
    callback func = nullptr;
};

command msgDispatcher(const json& body);

////////////////////////////////////////////////////////////////////////////////

struct food
{
    std::string name;
    enum {ANONYMOUS, NAME, QQ} offererType;
    struct { std::string name; int64_t qq; } offerer;
    std::string to_string(int64_t group = 0);
};
//inline std::vector<food> foodList;
void foodCreateTable();
//void foodLoadListFromDb();

struct drink
{
    std::string name;
	int64_t qq;
	int64_t group;
};
inline std::vector<drink> drinkList;
void drinkCreateTable();

}
