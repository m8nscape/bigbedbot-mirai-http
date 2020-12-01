#pragma once

#include <string>
#include <map>
#include <any>
#include <functional>

#define BOOST_ASIO_DISABLE_CONCEPTS
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace mirai::ws
{
void set_port(unsigned short port);

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

int connect(const std::string& path);
int disconnect();

void setRecvCallback(std::function<void(const std::string& msg)> cb);

}
