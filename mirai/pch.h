#pragma once

#include <string>
#include <vector>
#include <functional>
#include <map>
#include <sstream>
#include <future>
#include <exception>
#include <memory>

#include <nlohmann/json.hpp>

#define BOOST_ASIO_DISABLE_CONCEPTS
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
