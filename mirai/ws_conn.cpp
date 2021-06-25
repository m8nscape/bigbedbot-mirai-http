#include <vector>
#include <memory>
#include <exception>
#include <future>

#include "ws_conn.h"

#include "core.h"
#include "api.h"
#include "utils/logger.h"

namespace mirai::ws
{

const char* HOST = "localhost";
char PORT[6] = "12345";

void set_port(unsigned short port)
{
    snprintf(PORT, sizeof(PORT), "%hu", port);
}

std::shared_ptr<net::io_context> ioc;
std::shared_ptr<websocket::stream<tcp::socket>> ws;
std::function<void(const std::string& msg)> recvCallback{[](const std::string&){}};

bool listening = false;
std::promise<int> listenPromise;
std::future<int> listenFuture;
std::shared_ptr<std::thread> msg_poll_thread = nullptr;
int reconnectTimeout = 5;

int connect(const std::string& path)
{
    ioc = std::make_shared<net::io_context>();
    tcp::resolver resolver{*ioc};
    ws = std::make_shared<websocket::stream<tcp::socket>>(*ioc);

    try
    {
        auto const results = resolver.resolve(HOST, PORT);
        net::connect(ws->next_layer(), results.begin(), results.end());

        ws->set_option(websocket::stream_base::decorator(
        [](websocket::request_type& req)
        {
            req.set(http::field::user_agent,
                std::string(BOOST_BEAST_VERSION_STRING) +
                    " websocket-client-coro");
        }));

        ws->handshake(HOST, path);
    }
    catch (std::exception const& e)
    {
        addLog(LOG_ERROR, "ws", "%s", e.what());
        ws.reset();
        ioc.reset();
        return -1;
    }

    listening = true;
    reconnectTimeout = 5;
    listenFuture = listenPromise.get_future();
    msg_poll_thread = std::make_shared<std::thread>([]
    {
        while (listening && ws && ws->is_open())
        {
            beast::flat_buffer buffer;
            try
            {
                ws->read(buffer);
                if (buffer.size() > 0)
                    recvCallback(beast::buffers_to_string(buffer.data()));
            }
            catch (std::exception const& e)
            {
                if (listening)
                {
                    addLog(LOG_WARNING, "ws", "Exception occurred while reading: %s", e.what());

                    if (reconnectTimeout > 0)
                    {
                        bool reconnectSucceed = false;
                        while (!reconnectSucceed)
                        {
                            addLog(LOG_INFO, "ws", "Reconnecting in %d secounds", reconnectTimeout);
                            std::this_thread::sleep_for(std::chrono::seconds(reconnectTimeout));

                            reconnectTimeout *= 2;
                            if (mirai::testConnection() != 0)
                            {
                                addLog(LOG_WARNING, "ws", "Test connection failed");
                                continue;
                            }
                        }

                        if (mirai::registerApp() < 0)
                        {
                            addLog(LOG_ERROR, "ws", "Authenticating with mirai core failed");
                            core::shutdown();
                        }
#ifdef NDEBUG
                        std::thread(mirai::connectMsgWebSocket).detach();
#endif
                    }
                    else
                    {
                        core::shutdown();
                    }
                }
                break;
            }
        }
        listenPromise.set_value(0);
    });
    msg_poll_thread->detach();

    return 0;
}

int disconnect()
{
    listening = false;
    if (ws) ws->close(websocket::close_code::normal);
    listenFuture.wait_for(std::chrono::seconds(3));
    ws.reset();
    ioc.reset();
    return 0;
}

void setRecvCallback(std::function<void(const std::string& msg)> cb)
{
    recvCallback = cb;
}

}