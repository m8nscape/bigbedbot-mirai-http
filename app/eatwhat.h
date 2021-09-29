#pragma once
#include <vector>
#include <map>
#include <string>
#include <functional>
#include <iostream>

#include "common/dbconn.h"
#include "nlohmann/json.hpp"

namespace eatwhat {

using nlohmann::json;

inline SQLite db("eat.db", "eat");

void msgDispatcher(const json& body);

////////////////////////////////////////////////////////////////////////////////

struct food
{
    std::string name;
    enum {ANONYMOUS, NAME, QQ} offererType;
    struct { std::string name; int64_t qq; } offerer;
    int64_t groupid;
    std::string to_string(int64_t group = 0);
};
//inline std::vector<food> foodList;
void init();
void foodCreateTable();
void foodLoadListFromDb();

struct drink
{
    std::string name;
	int64_t qq;
	int64_t group;
};
//inline std::vector<drink> drinkList;
void drinkCreateTable();
void drinkLoadListFromDb();

}
