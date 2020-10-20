#pragma once
#include <vector>
#include <string>
#include <map>
#include <functional>

#include "nlohmann/json.hpp"

namespace opencase
{

using nlohmann::json;
void init(const char* case_list_yml);

enum class commands : size_t {
	开箱,
	开箱10,
	开黄箱,
	开红箱,
	开箱endless,
};

void msgDispatcher(const json& body);

////////////////////////////////////////////////////////////////////////////////
// 开箱
const int FEE_PER_CASE = 17;
class case_type
{
private:
	std::string _name;
	//int _worth;
	double _prob;
public:
	case_type(std::string n, /*int w,*/ double p) : _name(n), /*_worth(w),*/ _prob(p) {}
	std::string name() const { return _name; }
	//int worth() const { return _worth; }
	double prob() const { return _prob; }
	//operator int() const { return worth(); }
};

class case_detail
{
private:
	size_t _type_idx;
	std::string _name;
	int _worth = 0;
public:
	//case_detail() : _type_idx(CASE_POOL.size()) {}
	case_detail();
	case_detail(size_t ty, std::string n, int w) : _type_idx(ty), _name(n), _worth(w) {}
	size_t type_idx() const { return _type_idx; }
	const case_type& type() const;
	std::string name() const { return _name; }
	std::string part() const { using namespace std::string_literals; return !_name.empty() ? _name : type().name(); }
	std::string full() const { using namespace std::string_literals; return type().name() + (!_name.empty() ? (" [ "s + _name + " ]"s) : ""); }
	int worth() const { return _worth; }
	operator int() const { return worth(); }
};

const case_detail& draw_case(double p);

////////////////////////////////////////////////////////////////////////////////
// 技能设定
const int COST_OPEN_RED = 50;
const int COST_OPEN_YELLOW = 255;

const int COST_OPEN_ENDLESS = 50;

const int COST_OPEN_RED_STAMINA = 6;
const int COST_OPEN_ENDLESS_STAMINA = 7;

}