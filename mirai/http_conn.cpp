#include <vector>

#include "http_conn.h"

#include "utils/logger.h"

namespace mirai::http
{

const char* HOST = "localhost";
char PORT[6] = "12345";

void set_port(unsigned short port)
{
    snprintf(PORT, sizeof(PORT), "%hu", port);
}

const json respFailBody = R"({"code": -1})"_json;

// Performs an HTTP GET and prints the response
class session : public std::enable_shared_from_this<session>
{
    tcp::resolver resolver_;
    tcp::socket socket_;
    boost::beast::flat_buffer buffer_; // (Must persist between reads)
    http::request<http::string_body> req_;
    http::response<http::string_body> res_;

    std::function<int(const json&)> callback;

public:
    // Resolver and socket require an io_context
    explicit
    session(boost::asio::io_context& ioc)
        : resolver_(ioc)
        , socket_(ioc)
    {
    }

    // Start the asynchronous operation
    void
    GET(
        char const* host,
        char const* port,
        char const* target,
        std::function<int(const json&)> cb,
        int version = 10)
    {
        callback = cb;

        // Set up an HTTP GET request message
        req_.version(version);
        req_.method(http::verb::get);
        req_.target(target);
        req_.set(http::field::host, host);
        req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Look up the domain name
        resolver_.async_resolve(
            host,
            port,
            std::bind(
                &session::on_resolve,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2));
    }

    // Start the asynchronous operation
    void
    POST(
        char const* host,
        char const* port,
        char const* target,
        json const& body,
        std::function<int(const json&)> cb,
        int version = 10)
    {
        callback = cb;

        // Set up an HTTP POST request message
        req_.version(version);
        req_.method(http::verb::post);
        req_.target(target);
        req_.set(http::field::host, host);
        req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        if (!body.empty())
        {
            auto b = body.dump();
            req_.set(http::field::content_type, "application/json;charset=UTF-8");
            req_.set(http::field::content_length, std::to_string(b.length()));
            req_.body() = b;
        }

        // Look up the domain name
        resolver_.async_resolve(
            host,
            port,
            std::bind(
                &session::on_resolve,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2));
    }

    void
    on_resolve(
        boost::system::error_code ec,
        tcp::resolver::results_type results)
    {
        if(ec)
        {
            callback(respFailBody);
            return;
        }

        // Make the connection on the IP address we get from a lookup
        boost::asio::async_connect(
            socket_,
            results.begin(),
            results.end(),
            std::bind(
                &session::on_connect,
                shared_from_this(),
                std::placeholders::_1));
    }

    void
    on_connect(boost::system::error_code ec)
    {
        if(ec)
        {
            callback(respFailBody);
            return;
        }

        // Send the HTTP request to the remote host
        http::async_write(socket_, req_,
            std::bind(
                &session::on_write,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2));
    }

    void
    on_write(
        boost::system::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
        {
            callback(respFailBody);
            return;
        }
        
        // Receive the HTTP response
        http::async_read(socket_, buffer_, res_,
            std::bind(
                &session::on_read,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2));
    }

    void
    on_read(
        boost::system::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
        {
            callback(respFailBody);
            return;
        }

        // callback
        callback(json::parse(res_.body()));

        // Gracefully close the socket
        socket_.shutdown(tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes so don't bother reporting it.
        if(ec && ec != boost::system::errc::not_connected)
        {
            callback(respFailBody);
            return;
        }

        // If we get here then the connection is closed gracefully
    }
};

static unsigned get_count = 0;
int GET(const std::string& target, std::function<int(const json&)> callback)
{
    unsigned c = ++get_count;
    net::io_context ioc;
    //addLogDebug("http", "GET[%u] %s", c, target.c_str());
    std::make_shared<session>(ioc)->GET(HOST, PORT, target.c_str(), 
        [c, &callback](const json& j)
        {
            //addLogDebug("http", "GET[%u] resp %s", c, j.dump().c_str());
            return callback(j);
        }, 11);
    ioc.run();
    return 0;
}

static int post_count = 0;
int POST(const std::string& target, const json& body, std::function<int(const json&)> callback)
{
    unsigned c = ++post_count;
    net::io_context ioc;
    //addLogDebug("http", "POST[%u] %s %s", c, target.c_str(), body.dump().c_str());
    std::make_shared<session>(ioc)->POST(HOST, PORT, target.c_str(), body, 
        [c, &callback](const json& j)
        {
            //addLogDebug("http", "POST[%u] resp %s", c, j.dump().c_str());
            return callback(j);
        }, 11);
    ioc.run();
    return 0;
}

}