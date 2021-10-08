#pragma once
#include <any>
#include <vector>
#include <exception>
#include <future>
#include <string>

struct sqlite3;

#define SQLITE_OK 0

class SQLite
{
private:
    sqlite3* _db = nullptr;
    char logGrp[32] = {0};
    bool inTransaction = false;
public:
    SQLite() = delete;
    SQLite(const char* path, const char* log);
    ~SQLite();
    std::vector<std::vector<std::any>> query(const char* stmt, size_t retSize);
    std::vector<std::vector<std::any>> query(const char* stmt, size_t retSize, std::initializer_list<std::any> args);
    std::vector<std::vector<std::any>> query(const std::string& stmt, size_t retSize) { return query(stmt.c_str(), retSize); }
    std::vector<std::vector<std::any>> query(const std::string& stmt, size_t retSize, std::initializer_list<std::any> args) { return query(stmt.c_str(), retSize, args); }
	int exec(const char* zsql);
    int exec(const char* zsql, std::initializer_list<std::any> args);
	int exec(const std::string& zsql) { return exec(zsql.c_str()); }
    int exec(const std::string& zsql, std::initializer_list<std::any> args) { return exec(zsql.c_str(), args); }
    void transactionStart();
    void transactionStop();
    void commit(bool restart_transaction = false);
    const char* errmsg();
};
