#include "utils/logger.h"
#include "api.h"

namespace bbb::api
{

bool ApiHttpWorker::process_head(boost::beast::string_view target, const std::string& body)
{
    std::string strTarget(target.data(), target.length());
    addLog(LOG_ERROR, "bbbapi", "HEAD %s | %s", strTarget.c_str(), body.c_str());
    send_text_response(beast::http::status::not_found, "");
    return true;
}

bool ApiHttpWorker::process_get(boost::beast::string_view target, const std::string& body)
{
    std::string strTarget(target.data(), target.length());
    addLog(LOG_ERROR, "bbbapi", "GET %s | %s", strTarget.c_str(), body.c_str());
    send_text_response(beast::http::status::not_found, "404");
    return true;
}

bool ApiHttpWorker::process_post(boost::beast::string_view target, const std::string& body)
{
    std::string strTarget(target.data(), target.length());
    addLog(LOG_ERROR, "bbbapi", "POST %s | %s", strTarget.c_str(), body.c_str());
    send_text_response(beast::http::status::not_found, "404");
    return true;
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