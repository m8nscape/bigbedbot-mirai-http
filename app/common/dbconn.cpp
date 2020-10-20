#include <cstring>
#include <string>
#include <thread>
#include <cstdint>

#include "dbconn.h"

#define SQLITE_INT64_TYPE int64_t
#define SQLITE_UINT64_TYPE uint64_t
#include "sqlite3/sqlite3.h"

#include "logger.h"

SQLite::SQLite(const char* path, const char* log) { strcpy(logGrp, log); sqlite3_open(path, &_db); }
SQLite::~SQLite() { sqlite3_close(_db); }
const char* SQLite::errmsg() { return sqlite3_errmsg(_db); }

std::vector<std::vector<std::any>> SQLite::query(const char* zsql, size_t retSize)
{
    sqlite3_stmt* stmt = nullptr;
    const char* pzTail;
    if (int ret = sqlite3_prepare_v3(_db, zsql, strlen(zsql), 0, &stmt, &pzTail))
        return {};

    std::vector<std::vector<std::any>> ret;
    size_t idx = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        ret.push_back({});
        ret[idx].resize(retSize);
        for (size_t i = 0; i < retSize; ++i)
        {
            auto c = sqlite3_column_type(stmt, i);
            if (SQLITE_INTEGER == c)
                ret[idx][i] = sqlite3_column_int64(stmt, i);
            else if (SQLITE_FLOAT == c)
                ret[idx][i] = sqlite3_column_double(stmt, i);
            else if (SQLITE_TEXT == c)
                ret[idx][i] = std::make_any<std::string>((const char*)sqlite3_column_text(stmt, i));
        }   
        ++idx;
    }
    char msg[128];
    sprintf(msg, "query result: %u rows", ret.size());
    addLogDebug(logGrp, msg);
    sqlite3_finalize(stmt);
    return ret;
}
std::vector<std::vector<std::any>> SQLite::query(const char* zsql, size_t retSize, std::initializer_list<std::any> args)
{
    size_t argc = args.size();

    sqlite3_stmt* stmt = nullptr;
    const char* pzTail;
    if (int ret = sqlite3_prepare_v3(_db, zsql, strlen(zsql), 0, &stmt, &pzTail))
        return {};

    int i = 1;
    for (auto& a : args)
    {
        if (a.type() == typeid(int)) sqlite3_bind_int(stmt, i, std::any_cast<int>(a));
        else if (a.type() == typeid(int64_t)) sqlite3_bind_int64(stmt, i, std::any_cast<int64_t>(a));
        else if (a.type() == typeid(time_t)) sqlite3_bind_int64(stmt, i, std::any_cast<time_t>(a));
        else if (a.type() == typeid(double)) sqlite3_bind_double(stmt, i, std::any_cast<double>(a));
        else if (a.type() == typeid(std::string)) sqlite3_bind_text(stmt, i, std::any_cast<std::string>(a).c_str(), std::any_cast<std::string>(a).length(), SQLITE_TRANSIENT);
        else if (a.type() == typeid(const char*)) sqlite3_bind_text(stmt, i, std::any_cast<const char*>(a), strlen(std::any_cast<const char*>(a)), SQLITE_TRANSIENT);
        else if (a.type() == typeid(nullptr)) sqlite3_bind_null(stmt, i);
        ++i;
    }

    std::vector<std::vector<std::any>> ret;
    size_t idx = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        ret.push_back({});
        ret[idx].resize(retSize);
        for (size_t i = 0; i < retSize; ++i)
        {
            auto c = sqlite3_column_type(stmt, i);
            if (SQLITE_INTEGER == c)
                ret[idx][i] = sqlite3_column_int64(stmt, i);
            else if (SQLITE_FLOAT == c)
                ret[idx][i] = sqlite3_column_double(stmt, i);
            else if (SQLITE_TEXT == c)
                ret[idx][i] = std::make_any<std::string>((const char*)sqlite3_column_text(stmt, i));
        }
        ++idx;
    }
    char msg[128];
    sprintf(msg, "query result: %u rows", ret.size());
    addLogDebug(logGrp, msg);
    sqlite3_finalize(stmt);
    return ret;
}

int SQLite::exec(const char* zsql)
{
	sqlite3_stmt* stmt = nullptr;
	const char* pzTail;
	int ret;
	if (ret = sqlite3_prepare_v3(_db, zsql, strlen(zsql), 0, &stmt, &pzTail))
		return ret;
	if ((ret = sqlite3_step(stmt)) != SQLITE_OK && ret != SQLITE_ROW && ret != SQLITE_DONE)
	{
		char msg[1024];
		sprintf(msg, "%s %s: %s", "exec", zsql, errmsg());
		addLog(LOG_ERROR, logGrp, msg);
	}
	sqlite3_finalize(stmt);
	return SQLITE_OK;
}
int SQLite::exec(const char* zsql, std::initializer_list<std::any> args)
{
    sqlite3_stmt* stmt = nullptr;
    const char* pzTail;
    int ret;
    if (ret = sqlite3_prepare_v3(_db, zsql, strlen(zsql), 0, &stmt, &pzTail))
        return ret;

    int i = 1;
    for (auto& a : args)
    {
        if (a.type() == typeid(int)) sqlite3_bind_int(stmt, i, std::any_cast<int>(a));
        else if (a.type() == typeid(int64_t)) sqlite3_bind_int64(stmt, i, std::any_cast<int64_t>(a));
        else if (a.type() == typeid(time_t)) sqlite3_bind_int64(stmt, i, std::any_cast<time_t>(a));
        else if (a.type() == typeid(double)) sqlite3_bind_double(stmt, i, std::any_cast<double>(a));
        else if (a.type() == typeid(std::string)) sqlite3_bind_text(stmt, i, std::any_cast<std::string>(a).c_str(), std::any_cast<std::string>(a).length(), SQLITE_TRANSIENT);
        else if (a.type() == typeid(const char*)) sqlite3_bind_text(stmt, i, std::any_cast<const char*>(a), strlen(std::any_cast<const char*>(a)), SQLITE_TRANSIENT);
        else if (a.type() == typeid(nullptr)) sqlite3_bind_null(stmt, i);
        ++i;
    }

    if ((ret = sqlite3_step(stmt)) != SQLITE_OK && ret != SQLITE_ROW && ret != SQLITE_DONE)
    {
        char msg[1024];
        sprintf(msg, "%s %s: %s", "exec", zsql, errmsg());
        addLog(LOG_ERROR, logGrp, msg);
    }
    sqlite3_finalize(stmt);
    return SQLITE_OK;
}

void SQLite::transactionStart()
{
    if (!inTransaction)
        inTransaction = true;
    else
        return;

    sqlite3_stmt* stmt = nullptr;
    const char* pzTail;
    if (int ret = sqlite3_prepare_v3(_db, "BEGIN", 6, 0, &stmt, &pzTail))
        return;
    int ret;
    if ((ret = sqlite3_step(stmt)) != SQLITE_OK && ret != SQLITE_ROW && ret != SQLITE_DONE)
    {
        char msg[256];
        sprintf(msg, "%s: %s", "transactionStart", errmsg());
        addLog(LOG_ERROR, logGrp, msg);
    }
    else
    {
        //addLogDebug(logGrp, "transaction started");
    }
    sqlite3_finalize(stmt);
}
void SQLite::transactionStop()
{
    if (inTransaction)
        inTransaction = false;
    else
        return;

    sqlite3_stmt* stmt = nullptr;
    const char* pzTail;
    if (int ret = sqlite3_prepare_v3(_db, "COMMIT", 6, 0, &stmt, &pzTail))
        return;
    int ret;
    if ((ret = sqlite3_step(stmt)) != SQLITE_OK && ret != SQLITE_ROW && ret != SQLITE_DONE)
    {
        char msg[256];
        sprintf(msg, "%s: %s", "transactionEnd", errmsg());
        addLog(LOG_ERROR, logGrp, msg);
    }
    else
    {
        //addLogDebug(logGrp, "transaction finished");
    }
    sqlite3_finalize(stmt);
}
void SQLite::commit(bool restart_transaction)
{
    transactionStop();
    if (restart_transaction) transactionStart();
}
