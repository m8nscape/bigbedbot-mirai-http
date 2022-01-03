#pragma once
#include <map>
#include <functional>
#include <nlohmann/json.hpp>

namespace gambol
{
// gambol (x)
// gamble (o)

///////////////////////////////////////////////////////////////////////////////

enum class commands : size_t {
    // basic
    BET,

    // flipcoin
    FLIPCOIN_START,
    FLIPCOIN_FRONT,
    FLIPCOIN_BACK,
    ROULETTE_START,
    ROULETTE_CHOOSE,
};

inline std::map<std::string, commands> commands_str
{
    {"开始翻批", commands::FLIPCOIN_START},
    {"開始翻批", commands::FLIPCOIN_START}, //繁體化
    {"正", commands::FLIPCOIN_FRONT},       //簡體繁體一樣
    {"反", commands::FLIPCOIN_BACK},        //簡體繁體一樣
    {"开始摇号", commands::ROULETTE_START},
    {"開始搖號", commands::ROULETTE_START}, //繁體化
    {"摇号", commands::ROULETTE_CHOOSE},
    {"搖號", commands::ROULETTE_CHOOSE},    //繁體化
    {"摇", commands::ROULETTE_CHOOSE},
    {"搖", commands::ROULETTE_CHOOSE}       //繁體化
};

void msgDispatcher(const nlohmann::json& body);

///////////////////////////////////////////////////////////////////////////////
// flipcoin
namespace flipcoin
{

struct bet
{
    int64_t front;
    int64_t back;
};
struct game
{
    int64_t total = 0, front = 0, back = 0;
    std::map<int64_t, bet> pee_per_player;
    time_t startTime = 0;
};

void roundStart(int64_t group);
void roundAnnounce(int64_t group);
void roundEnd(int64_t group);
void roundCancel(int64_t group);
void roundCancelAll();

void put(int64_t group, int64_t qq, bet s);

}

///////////////////////////////////////////////////////////////////////////////
// roulette
namespace roulette
{

enum grid
{
    N0,  N1,  N2,  N3,  N4,  N5,  N6,  N7,
    N8,  N9,  N10, N11, N12, N13, N14, N15,
    N16, N17, N18, N19, N20, N21, N22, N23,
    N24, N25, N26, N27, N28, N29, N30, N31,
    N32, N33, N34, N35, N36,
    Cblack, // 黑
    Cred,   // 红
    Aodd,   // 单数
    Aeven,  // 双数
    P1st, P2nd, P3rd,
    GRID_COUNT
};

inline const char* gridName[] = 
{
    "0",  "1",  "2",  "3",  "4",  "5",  "6",
    "7",  "8",  "9",  "10", "11", "12", "13",
    "14", "15", "16", "17", "18", "19", "20",
    "21", "22", "23", "24", "25", "26", "27",
    "28", "29", "30", "31", "32", "33", "34",
    "35", "36",
    "黑", "红",
    "单", "双",
    "1st", "2nd", "3rd"
};

inline const std::map<std::string, grid> gridTokens
{
    {"0", N0},   {"1", N1},   {"2", N2},   {"3", N3},
    {"4", N4},   {"5", N5},   {"6", N6},   {"7", N7},
    {"8", N8},   {"9", N9},   {"10", N10}, {"11", N11},
    {"12", N12}, {"13", N13}, {"14", N14}, {"15", N15},
    {"16", N16}, {"17", N17}, {"18", N18}, {"19", N19},
    {"20", N20}, {"21", N21}, {"22", N22}, {"23", N23},
    {"24", N24}, {"25", N25}, {"26", N26}, {"27", N27},
    {"28", N28}, {"29", N29}, {"30", N30}, {"31", N31},
    {"32", N32}, {"33", N33}, {"34", N34}, {"35", N35},
    {"36", N36},
    {"黑", Cblack}, {"黑色", Cblack},
    {"红", Cred},  {"紅", Cred}, {"红色", Cred}, {"紅色", Cred},
    {"单", Aodd},
    {"奇", Aodd},
    {"单数", Aodd},
    {"单數", Aodd},
    {"奇数", Aodd},
    {"奇數", Aodd},
    {"双", Aeven},
    {"雙", Aeven},
    {"偶", Aeven},
    {"双数", Aeven},
    {"雙數", Aeven},
    {"偶数", Aeven},
    {"偶數", Aeven},
    {"1st", P1st}, {"2nd", P2nd}, {"3rd", P3rd},
    {"1ST", P1st}, {"2ND", P2nd}, {"3RD", P3rd},
};

struct bet
{
    int64_t amount[GRID_COUNT];
};
struct game
{
    int64_t total = 0, amount[GRID_COUNT]{ 0 };
    std::map<int64_t, bet> pee_per_player;
    time_t startTime = 0;
};

void roundStart(int64_t group);
void roundAnnounce(int64_t group);
void roundEnd(int64_t group);
void roundCancel(int64_t group);
void roundCancelAll();

void put(int64_t group, int64_t qq, grid g, int64_t amount);
}

struct gameData
{
	bool flipcoin_running;
	flipcoin::game flipcoin;

	bool roulette_running;
	roulette::game roulette;
};

extern std::map<int64_t, gameData> groupMap;
}
