#pragma once
#include <map>
#include <ctime>
#include <cstdint>

#include "nlohmann/json.hpp"

#include "../common/dbconn.h"

namespace user {

inline SQLite db("pee.db", "pee");

void init(const char* user_alias_yml);

// %%%%%%%%%%%%%%%% 批相关数据 %%%%%%%%%%%%%%%%
class pdata
{
public:
	pdata() {}
	pdata(int64_t qq): qq(qq) {}
	pdata(int64_t q, int64_t c, int64_t bc, time_t dt, int64_t k):
		qq(q), currency(c), opened_box_count(bc), last_draw_time(dt), key_count(k) {}
	~pdata() = default;

protected:
	int64_t qq = -1;
    int64_t currency = 0;
    int64_t opened_box_count = 0;
    time_t  last_draw_time = 0;
    int64_t key_count = 0;
	time_t  stamina_recovery_time = 0;
	int		stamina_extra = 0;

public:
	friend void peeLoadFromDb();

public:
	struct resultStamina
	{
		bool	enough;
		int		staminaAfterUpdate;
		time_t	fullRecoverTime;
	};
	resultStamina getStamina(bool extra) const;
	int getExtraStamina() const;
	resultStamina modifyStamina(int delta = 0, bool extra = false);
	resultStamina testStamina(int cost = 0) const;

	int64_t getCurrency() const { return currency; }
	int64_t getKeyCount() const { return key_count; }
	time_t  getLastDrawTime() const { return last_draw_time; }
	void modifyCurrency(int64_t delta);
	void multiplyCurrency(double a);
	void modifyBoxCount(int64_t delta);
	void modifyDrawTime(time_t time);
	void modifyKeyCount(int64_t delta);

	int createAccount(int64_t qq, int64_t initialBalance);
};

inline std::map<int64_t, pdata> plist;

// %%%%%%%%%%%%%%%% 个人数据 %%%%%%%%%%%%%%%%
const int INITIAL_BALANCE = 200;

using nlohmann::json;
enum class commands : size_t
{
	REG_HINT,
	REG,
	BALANCE,
	DRAW_P,
	GEN_P,
};

void msgCallback(const json& body);

// %%%%%%%%%%%%%%%% 体力 %%%%%%%%%%%%%%%%
#ifdef _DEBUG
const int MAX_STAMINA = 100;
#else
const int MAX_STAMINA = 10;
#endif
const int STAMINA_TIME = 30 * 60; // 30min

// %%%%%%%%%%%%%%%% 领批 %%%%%%%%%%%%%%%%
const int FREE_BALANCE_ON_NEW_DAY = 10;
const int NEW_DAY_TIME_HOUR = 11;
const int NEW_DAY_TIME_MIN = 0;
const int DAILY_BONUS = 200;

inline int extra_tomorrow = 0;

inline time_t daily_refresh_time;   // 可领批的时间，最后领批时间戳需要小于这个时间
inline std::tm daily_refresh_tm;
inline std::tm daily_refresh_tm_auto;
inline int remain_daily_bonus;

void flushDailyTimep(bool autotriggered = false);

// %%%%%%%%%%%%%%%% 昵称 %%%%%%%%%%%%%%%%
int loadUserAlias(const char* yaml);
int64_t getUser(const std::string& alias);

// %%%%%%%%%%%%%%%% 盒盒 %%%%%%%%%%%%%%%%
class city_data
{
public:
    city_data() {}
    city_data(int64_t my_qq): qq(my_qq) {}
    city_data(int64_t my_qq, int64_t my_city_id):
        qq(my_qq), city_id(my_city_id) {}
    ~city_data() = default;

    int64_t get_city_id() {return city_id;}

private:
    int64_t qq = -1;
    int64_t city_id = 0;
}
}