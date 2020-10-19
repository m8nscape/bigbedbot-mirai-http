#pragma once
#include <string>
#include <vector>
#include <map>
#include <functional>
namespace help {

enum class commands : size_t {
	help
};

inline std::map<std::string, commands> commands_str
{
	{"°ïÖú", commands::help},
	{"ŽÍÖú", commands::help},
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

std::string boot_info();

std::string help();

}