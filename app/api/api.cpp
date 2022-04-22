#include "utils/logger.h"
#include "api.h"
#include <nlohmann/json.hpp>
#include <codecvt>

#if __GNUC__ >= 8
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

#include "api_user.h"

namespace bbb::api
{

ApiHttpWorker::ApiHttpWorker(tcp::acceptor& acceptor) : http_worker(acceptor)
{
    add_proc_get("/user/info", user::get_info);
    add_proc_get("/user/group_info", user::get_group_info);
    add_proc_post("/user/give_currency", user::post_give_currency);
    add_proc_post("/user/take_currency", user::post_take_currency);
}

void ApiKeyObject::add(const std::string& ak, AKLevel level)
{
    if (ak_objects.find(ak) == ak_objects.end())
    {
        ak_objects[ak].level = level;
        addLog(LOG_ERROR, "bbbapi", "Lv%d: %s", level, ak.data());
    }
    else
    {
        addLog(LOG_ERROR, "bbbapi", "Key is already added %s", ak.data());
    }
}

bool ApiKeyObject::has_key(const std::string& ak)
{
    return ak_objects.find(ak) != ak_objects.end();
}

bool ApiKeyObject::verify_freq_head(const std::string& ak)
{
    assert(has_key(ak));
    auto& obj = ak_objects[ak];
    if (obj.level == AKLevel::MASTER) return true;

    bool ret = true;
    int slice_length;
    switch (obj.level)
    {
        case AKLevel::NORMAL: slice_length = 200;
        case AKLevel::HIFREQ: slice_length = 80;
    }
    int64_t slice_idx = Clock::now().time_since_epoch().count() / slice_length;
    if (slice_idx != obj.slice_idx)
    {
        obj.current_slice_request_count = 0;
    }
    
    obj.current_slice_request_count++;
    return (obj.current_slice_request_count <= MAX_ONE_SLICE_REQUEST_COUNT);
}

bool ApiKeyObject::verify_freq_get(const std::string& ak)
{
    assert(has_key(ak));
    auto& obj = ak_objects[ak];
    if (obj.level == AKLevel::MASTER) return true;

    bool ret = true;
    int slice_length;
    switch (obj.level)
    {
        case AKLevel::NORMAL: slice_length = 500;
        case AKLevel::HIFREQ: slice_length = 100;
    }
    int64_t slice_idx = Clock::now().time_since_epoch().count() / slice_length;
    if (slice_idx != obj.slice_idx)
    {
        obj.current_slice_request_count = 0;
    }
    
    obj.current_slice_request_count++;
    return (obj.current_slice_request_count <= MAX_ONE_SLICE_REQUEST_COUNT);
}

bool ApiKeyObject::verify_freq_post(const std::string& ak)
{
    assert(has_key(ak));
    auto& obj = ak_objects[ak];
    if (obj.level == AKLevel::MASTER) return true;

    bool ret = true;
    int slice_length;
    switch (obj.level)
    {
        case AKLevel::NORMAL: slice_length = 1000;
        case AKLevel::HIFREQ: slice_length = 200;
    }
    int64_t slice_idx = Clock::now().time_since_epoch().count() / slice_length;
    if (slice_idx != obj.slice_idx)
    {
        obj.current_slice_request_count = 0;
    }
    
    obj.current_slice_request_count++;
    return (obj.current_slice_request_count <= MAX_ONE_SLICE_REQUEST_COUNT);
}

std::string unescape(std::string_view s)
{
    std::stringstream ss;
    size_t len = s.length();
    for (size_t i = 0; i < len; ++i)
    {
        if (s[i] == '%' && s[i + 1] == 'u' && i < len - 5)
        {
            char buf[8] = {0};
            memcpy(buf, &s[i] + 2, 4);
            char16_t u16 = (char16_t)std::strtol(buf, NULL, 16);
            static std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> cc;
            std::string u8_conv = cc.to_bytes(u16);
            ss << u8_conv;
        }
        else if (s[i] == '%' && i < len - 2)
        {
            char buf[4] = {0};
            memcpy(buf, &s[i] + 1, 2);
            char c = (char)std::strtol(buf, NULL, 16);
            ss << c;
        }
        else
        {
            ss << s[i];
        }
    }
    return ss.str();
}

std::pair<std::string, std::map<std::string, std::string>> get_url_params(std::string_view target)
{
    std::size_t idxParam = target.find_first_of('?');
    std::string_view path = target.substr(0, idxParam);
    std::string_view param = target;
    std::map<std::string, std::string> params;
    if  (idxParam != target.npos)
    {
        size_t idxAnd;
        do
        {
            param = param.substr(idxParam + 1);
            idxAnd = param.find('&');
            std::string_view param0 = param.substr(0, idxAnd);
            size_t idxEqual = param0.find('=');
            params[unescape(param0.substr(0, idxEqual))] = unescape(param0.substr(idxEqual + 1));
            idxParam = idxAnd + 1;
        } while (idxAnd != path.npos);
    }
    return std::make_pair(std::string(path), params);
}

bool ApiHttpWorker::process_head(boost::beast::string_view target, const std::string& body)
{
    std::string strTarget(target.data(), target.length());
    addLog(LOG_ERROR, "bbbapi", "HEAD %s | %s", strTarget.c_str(), body.c_str());

    auto [path, params] = get_url_params(std::string_view(target.data(), target.length()));
    if (params["ak"].empty() || !ApiKeyObject::has_key(params["ak"]))
    {
        send_text_response(beast::http::status::forbidden, "");
        return true;
    }
    if (!ApiKeyObject::verify_freq_head(params["ak"]))
    {
        send_text_response(beast::http::status::too_many_requests, "");
        return true;
    }

    ProcProp proc = find_proc(path, node_head);
    if (std::get_if<ProcFunc>(&proc) != nullptr)
    {
        auto resp = std::get<ProcFunc>(proc)(strTarget, params, body);
        send_text_response(resp.first, resp.second);
        return true;
    }
    else if (std::get_if<std::string>(&proc) != nullptr)
    {
        send_text_response(beast::http::status::forbidden, "");
        return true;
    }
    else
    {
        send_text_response(beast::http::status::not_found, "");
        return true;
    }
}

bool ApiHttpWorker::process_get(boost::beast::string_view target, const std::string& body)
{
    std::string strTarget(target.data(), target.length());
    addLog(LOG_ERROR, "bbbapi", "GET %s | %s", strTarget.c_str(), body.c_str());

    auto [path, params] = get_url_params(std::string_view(target.data(), target.length()));
    if (params["ak"].empty() || !ApiKeyObject::has_key(params["ak"]))
    {
        send_text_response(beast::http::status::forbidden, "");
        return true;
    }
    if (!ApiKeyObject::verify_freq_get(params["ak"]))
    {
        send_text_response(beast::http::status::too_many_requests, "");
        return true;
    }
    
    ProcProp proc = find_proc(path, node_get);
    if (std::get_if<ProcFunc>(&proc) != nullptr)
    {
        auto resp = std::get<ProcFunc>(proc)(strTarget, params, body);
        send_text_response(resp.first, resp.second);
        return true;
    }
    else if (std::get_if<std::string>(&proc) != nullptr)
    {
        send_text_response(beast::http::status::forbidden, "");
        return true;
    }
    else
    {
        send_text_response(beast::http::status::not_found, "");
        return true;
    }
}

bool ApiHttpWorker::process_post(boost::beast::string_view target, const std::string& body)
{
    std::string strTarget(target.data(), target.length());
    addLog(LOG_ERROR, "bbbapi", "POST %s | %s", strTarget.c_str(), body.c_str());

    auto [path, params] = get_url_params(std::string_view(target.data(), target.length()));
    nlohmann::json jsonReq(body);
    if (jsonReq.find("ak") == jsonReq.end() || !ApiKeyObject::has_key(jsonReq["ak"].get<std::string>()))
    {
        send_text_response(beast::http::status::forbidden, "");
        return true;
    }
    if (!ApiKeyObject::verify_freq_post(jsonReq["ak"].get<std::string>()))
    {
        send_text_response(beast::http::status::too_many_requests, "");
        return true;
    }

    ProcProp proc = find_proc(path, node_post);
    if (std::get_if<ProcFunc>(&proc) != nullptr)
    {
        auto resp = std::get<ProcFunc>(proc)(strTarget, params, body);
        send_text_response(resp.first, resp.second);
        return true;
    }
    else if (std::get_if<std::string>(&proc) != nullptr)
    {
        send_text_response(beast::http::status::forbidden, "");
        return true;
    }
    else
    {
        send_text_response(beast::http::status::not_found, "");
        return true;
    }
}

ApiHttpWorker::ProcProp ApiHttpWorker::find_proc(std::string_view path, ApiHttpWorker::ProcNode& node)
{
    if (path.empty() || path[0] != '/') 
        return ProcProp();

    std::string key(path);
    if (node.find(key) == node.end())
        return ProcProp();

    return node.at(key);
}

void ApiHttpWorker::add_proc(std::string_view path, const ProcProp& prop, ProcNode& node)
{
    node[std::string(path)] = prop;
}

int ApiInstance::init(const char* cfgFile)
{
    fs::path cfgPath(cfgFile);
    if (!fs::is_regular_file(cfgPath))
    {
        addLog(LOG_ERROR, "bbbapi", "Config file %s not found", fs::absolute(cfgPath).c_str());
        return -1;
    }
    addLog(LOG_INFO, "bbbapi", "Loading aks from %s", fs::absolute(cfgPath).c_str());
    YAML::Node cfg = YAML::LoadFile(cfgFile);
    int count = 0;
    for (auto ak: cfg["ak"])
    {
        if (ak.size() == 2)
        {
            try
            {
                std::string key = ak[0].as<std::string>();
                int level = ak[1].as<int>();
                ApiKeyObject::add(key, static_cast<ApiKeyObject::AKLevel>(level));
                count++;
            }
            catch(const std::exception& e)
            {
                addLog(LOG_WARNING, "bbbapi", e.what());
            }
        }
    }
    addLog(LOG_INFO, "bbbapi", "Added %d keys", count);
    return 0;
}

void ApiInstance::set_port(unsigned short port)
{
    this->port = port;
}

void ApiInstance::start()
{
    if (port < 1024)
    {
        addLog(LOG_ERROR, "bbbapi", "invalid port: %hd", port);
        return;
    }
    
    std::thread(ApiInstance::start_server, addr, port, 8, &quit, &running).detach();
}

void ApiInstance::start_server(std::string addr, unsigned short port, int num_workers, bool* quit, bool* running)
{
    boost::asio::io_context ioc{1};
    tcp::acceptor acceptor{ioc, {boost::asio::ip::make_address(addr), port}};

    *running = true;

    addLog(LOG_ERROR, "bbbapi", "Start listening on port %hd with %d workers", port, num_workers);
    std::list<ApiHttpWorker> workers;
    for (int i = 0; i < num_workers; ++i)
    {
        workers.emplace_back(acceptor);
        workers.back().start();
    }

    while (!*quit)
    {
        ioc.poll();
    }

    *running = false;
}

void ApiInstance::stop()
{
    quit = true;
    while (running)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

}