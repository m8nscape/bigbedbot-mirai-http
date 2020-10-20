#pragma once

#include <string>
#include <map>
#include <any>
#include <functional>

#define BOOST_ASIO_DISABLE_CONCEPTS
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <nlohmann/json.hpp>

namespace mirai::conn
{
void set_port(unsigned short port);

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

using nlohmann::json;

int connect();
int disconnect();

int GET(const std::string& path, std::function<int(const json&)> callback);
int POST(const std::string& path, const json& body, std::function<int(const json&)> callback);

}
