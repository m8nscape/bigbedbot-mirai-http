#pragma once

#include "utils/http_worker.hpp"
#include <functional>
#include <utility>
#include <variant>
#include <string>
#include <chrono>

namespace bbb::api
{
class ApiKeyObject
{
public:
    enum class AKLevel
    {
        FORBIDDEN,
        NORMAL,
        HIFREQ,
        MASTER = 99
    };
    AKLevel level;

protected:
    using Clock = std::chrono::system_clock;
    static inline const int MAX_ONE_SLICE_REQUEST_COUNT = 3;
    mutable int64_t slice_idx = 0;
    mutable int current_slice_request_count = 0;
    mutable std::chrono::time_point<Clock> last_request_time;
    static int64_t ms_from_t(decltype(last_request_time) t) { return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - t).count(); }

public:
    static void add(const std::string& ak, AKLevel level);
    static bool has_key(const std::string& ak);
    static bool verify_freq_head(const std::string& ak);
    static bool verify_freq_get(const std::string& ak);
    static bool verify_freq_post(const std::string& ak);
};
inline static std::map<std::string, ApiKeyObject> ak_objects;    // ak: time

using ProcResp = std::pair<boost::beast::http::status, std::string>;
using ProcFunc = std::function<ProcResp(std::string_view, const std::map<std::string, std::string>, const std::string&)>;

class ApiHttpWorker: public http_worker
{
public:
    ApiHttpWorker(tcp::acceptor& acceptor);

public:
    virtual bool process_head(boost::beast::string_view target, const std::string& body) override;
    virtual bool process_get(boost::beast::string_view target, const std::string& body) override;
    virtual bool process_post(boost::beast::string_view target, const std::string& body) override;

protected:
    using ProcProp = std::variant<std::monostate, std::string, ProcFunc>;
    using ProcNode = std::map<std::string, ProcProp>;
    ProcNode node_head;
    ProcNode node_get;
    ProcNode node_post;
    ProcProp find_proc(std::string_view path, ProcNode& node);
    void add_proc(std::string_view path, const ProcProp& prop, ProcNode& node);

    std::map<std::string, std::string> parse_path_args;

public:
    void add_proc_head(std::string_view path, const ProcProp& prop) { add_proc(path, prop, node_head); }
    void add_proc_get(std::string_view path, const ProcProp& prop) { add_proc(path, prop, node_get); }
    void add_proc_post(std::string_view path, const ProcProp& prop) { add_proc(path, prop, node_post); }

protected:
    std::string get_user_info();
};

class ApiInstance
{
protected:
    std::string addr = "0.0.0.0";
    unsigned short port = 0;
    bool running = false;
    bool quit = false;

public:
    void set_port(unsigned short port);
    void start();
    void stop();

protected:
    static void start_server(std::string addr, unsigned short port, int num_workers, bool* quit, bool* running);
};
inline ApiInstance inst;

}