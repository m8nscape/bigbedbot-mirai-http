#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace playwhat
{
    
// Ordinary json parser tooks about 80MB+ RAM so I should do the things by hand.
class SteamAppListParser
{
private:
    enum class Estat
    {
        IDLE,
        KEY_LEFT,
        KEY_QUOTE,
        KEY_RIGHT,
        VALUE_LEFT,
        VALUE_NUM,
        VALUE_QUOTE,
        VALUE_RIGHT,
        PAIR_FINISH,
    } stat = Estat::IDLE;
    int brace_counter = 0;

    const char* c_pointer = NULL;
    char key[512];
    char value[512];
    char* kp = key;
    char* vp = value;

    long appid_buf = 0;

public:
    struct game {
        long appid;
        std::string name;
    };
    std::vector<game> games;
    bool available = false;

public:	
    int parse(const char* s);
    int proc_IDLE();
    int proc_KEY_LEFT();
    int proc_KEY_QUOTE();
    int proc_KEY_RIGHT();
    int proc_VALUE_LEFT();
    int proc_VALUE_NUM();
    int proc_VALUE_QUOTE();
    int proc_VALUE_RIGHT();
    int proc_PAIR_FINISH();
    void parse_fail();
};

void updateSteamGameList();

using nlohmann::json;
void msgCallback(const json& body);
}