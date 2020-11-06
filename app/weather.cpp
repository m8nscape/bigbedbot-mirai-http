#include <vector>
#include <string>
#include <sstream>
#include <regex>
#include <filesystem>

#include "weather.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

#include "common/dbconn.h"
#include "utils/logger.h"
#include "mirai/api.h"
#include "mirai/msg.h"

namespace weather
{

bool inQuery = false;

struct curl_buffer
{
    int length = 0;
    char content[CURL_MAX_WRITE_SIZE] = "";
};

size_t perform_write(void* buffer, size_t size, size_t count, void* stream)
{
    curl_buffer* buf = (curl_buffer*)stream;
    int newsize = size * count;
    memcpy(buf->content + buf->length, buffer, newsize);
    buf->length += newsize;
    return newsize;
}

int curl_get(const std::string& url, std::string& content, int timeout_sec = 5)
{
    if (inQuery)
    {
        content = "API is busy";
        return -2;
    }

    inQuery = true;

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        content = "curl init failed";
        inQuery = false;
        return -1;
    }

    curl_buffer curlbuf;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, perform_write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curlbuf);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_sec);
    memset(curlbuf.content, 0, CURL_MAX_WRITE_SIZE);

    if (int ret; CURLE_OK != (ret = curl_easy_perform(curl)))
    {
        using namespace std::string_literals;
        switch (ret)
        {
        case CURLE_OPERATION_TIMEDOUT:
            content = "网络连接超时";
            break;
            
        default:
            content = "网络连接失败("s + std::to_string(ret) + ")";
            break;
        }
        inQuery = false;
        curl_easy_cleanup(curl);
        return ret;
    }

    content = std::string(curlbuf.content, curlbuf.length);
    inQuery = false;
    return CURLE_OK;
}

///////////////////////////////////////////////////////////////////////////////

namespace weathercn
{
int TIMEOUT_SEC = 1;
SQLite db("weathercn_city.db", "weather");
std::string getReqUrl(const std::string& name)
{
    auto ret = db.query("select id from cityid where name=?", 1, { name });
    if (ret.empty()) return "";
    std::string id = std::any_cast<std::string>(ret[0][0]);

    char buf[128];
    snprintf(buf, sizeof(buf)-1,
        "http://t.weather.sojson.com/api/weather/city/%s",
        id.c_str());
    return buf;
}
}

namespace openweather
{
std::string APIKEY;
int TIMEOUT_SEC = 5;
std::string getReqUrl(const std::string& name)
{
    char buf[128];
    snprintf(buf, sizeof(buf)-1,
        "https://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s",
        name.c_str(),
        APIKEY.c_str());
    return buf;
}
}

///////////////////////////////////////////////////////////////////////////////

/*
std::string success(::int64_t, ::int64_t, std::vector<std::string>& args, const char*)
{
    std::stringstream ss;
    ss <<
        args[1] << "," << args[0] << ": " <<
        args[2] << ", " <<
        args[3] << "°C (" <<  args[4]<< "°C~" << args[5] << "°C), " <<
        args[6] << "%";
    return ss.str();
}
*/

enum class commands : size_t {
    _,
    全球天气,
    国内天气
};
const std::vector<std::pair<std::regex, commands>> commands_regex
{
    {std::regex(R"(^weather +(.+)$)", std::regex::optimize | std::regex::extended | std::regex::icase), commands::全球天气},
    {std::regex(R"(^(.+) +weather$)", std::regex::optimize | std::regex::extended | std::regex::icase), commands::全球天气},
    {std::regex(R"(^(.+) *天气$)", std::regex::optimize | std::regex::extended), commands::国内天气},
    {std::regex(R"(^(.+) *天氣$)", std::regex::optimize | std::regex::extended), commands::国内天气},
    {std::regex(R"(^天气 +(.+)$)", std::regex::optimize | std::regex::extended), commands::国内天气},
    {std::regex(R"(^天氣 +(.+)$)", std::regex::optimize | std::regex::extended), commands::国内天气},
};

void msgCallback(const json& body)
{
    auto query = mirai::messageChainToStr(body);
    if (query.empty()) return;

    commands c = commands::_;
    std::string city;
    for (const auto& [re, cmd]: commands_regex)
    {
        std::smatch res;
        if (std::regex_match(query, res, re))
        {
            c = cmd;
            city = res[1].str();
            break;
        }
    }
    if (c == commands::_) return;

    auto m = mirai::parseMsgMetadata(body);

    switch (c)
    {
    case commands::全球天气:
        weather_global(m, city);
        break;
    case commands::国内天气:
        weather_cn(m, city);
        break;
    default:
        break;
    }
}

void weather_global(const mirai::MsgMetadata& m, const std::string& city)
{
    //auto url = seniverse::getReqUrl(name);
    auto url = openweather::getReqUrl(city);
    std::string buf;
    if (CURLE_OK != curl_get(url, buf, openweather::TIMEOUT_SEC))
    {
        mirai::sendMsgRespStr(m, buf);
        return;
    }

    nlohmann::json json = nlohmann::json::parse(buf);
    try
    {
        if (json.contains("cod") && json["cod"] == 200 && json.contains("coord"))
        {
            std::vector<std::string> args;
            args.push_back(json["sys"]["country"]);
            args.push_back(json["name"]);
            args.push_back(json["weather"][0]["main"]);
            args.push_back(std::to_string(int(std::round(json["main"]["temp"].get<double>() - 273.15))));
            args.push_back(std::to_string(int(std::round(json["main"]["temp_min"].get<double>() - 273.15))));
            args.push_back(std::to_string(int(std::round(json["main"]["temp_max"].get<double>() - 273.15))));
            args.push_back(std::to_string(int(std::round(json["main"]["humidity"].get<double>() + 0))));

            std::stringstream ss;
            ss <<
                args[1] << ", " << args[0] << ": " <<
                args[2] << ", " <<
                args[3] << "°C (" << args[4] << "°C~" << args[5] << "°C), " <<
                args[6] << "%";
            
            mirai::sendMsgRespStr(m, ss.str().c_str());
        }
        else throw std::exception();
    }
    catch (...)
    {
        std::stringstream ss;
        ss << "请求失败：";
        if (json.contains("cod") && json.contains("message"))
            ss << json["message"].get<std::string>();
        else
            ss << "unknown";
        mirai::sendMsgRespStr(m, ss.str().c_str());
    }
}

void weather_cn(const mirai::MsgMetadata& m, const std::string& name)
{
    CURL* curl = NULL;

    auto url = weathercn::getReqUrl(name);
    if (url.empty())
    {
        mirai::sendMsgRespStr(m, "未找到该城市");
        return;
    }

    std::string buf;
    if (CURLE_OK != curl_get(url, buf, weathercn::TIMEOUT_SEC))
    {
        mirai::sendMsgRespStr(m, buf);
        return;
    }

    nlohmann::json json = nlohmann::json::parse(buf);
    try
    {
        if (json.contains("cityInfo"))
        {
            int ibuf;
            double dbuf;
            char buf[16];
            std::vector<std::string> args;

            args.push_back(json["cityInfo"]["parent"]);
            args.push_back(json["cityInfo"]["city"]);
            args.push_back(json["data"]["wendu"]);
            args.push_back(json["data"]["shidu"]);

            dbuf = json["data"]["pm25"];
            snprintf(buf, sizeof(buf)-1, "%.1f", dbuf);
            args.push_back(buf);

            dbuf = json["data"]["pm10"];
            snprintf(buf, sizeof(buf)-1, "%.1f", dbuf);
            args.push_back(buf);

            args.push_back(json["data"]["forecast"][0]["type"]);
            args.push_back(json["data"]["forecast"][0]["low"]);
            args.push_back(json["data"]["forecast"][0]["high"]);

            ibuf = json["data"]["forecast"][0]["aqi"];
            snprintf(buf, sizeof(buf)-1, "%d", ibuf);
            args.push_back(buf);

            std::stringstream ss;
            ss << args[0] << " " << args[1] << " " << args[6] << std::endl <<
                "温度：" << args[2] << "℃（" << args[7] << "，" << args[8] << "） " << std::endl <<
                "湿度：" << args[3] << std::endl <<
                "PM2.5: " << args[4] << std::endl << 
                "PM10: " << args[5] << std::endl << 
                "AQI: " << args[9];

            int aqi = atoi(args[9].c_str());
            if (0 <= aqi && aqi <= 50)
                ss << " 一级（优）";
            else if (51 <= aqi && aqi <= 100)
                ss << " 二级（良）";
            else if (101 <= aqi && aqi <= 150)
                ss << " 三级（轻度污染）";
            else if (151 <= aqi && aqi <= 200)
                ss << " 四级（中度污染）";
            else if (201 <= aqi && aqi <= 300)
                ss << " 五级（重度污染）";
            else if (301 <= aqi)
                ss << " 六级（严重污染）";
            else
                ss << " undefined（？）";

            mirai::sendMsgRespStr(m, ss.str().c_str());
        }
        else throw std::exception();
    }
    catch (...)
    {
        mirai::sendMsgRespStr(m, "天气解析失败");
    }
}

int init(const char* yaml)
{
    std::filesystem::path cfgPath(yaml);
    if (!std::filesystem::is_regular_file(cfgPath))
    {
        addLog(LOG_ERROR, "weather", "weather config file %s not found", std::filesystem::absolute(cfgPath).c_str());
        return -1;
    }
    addLog(LOG_INFO, "weather", "Loading weather config from %s", std::filesystem::absolute(cfgPath).c_str());

    YAML::Node cfg = YAML::LoadFile(yaml);

    // 配置
    openweather::APIKEY = cfg["openweather_apikey"].as<std::string>();
    openweather::TIMEOUT_SEC = cfg["openweather_timeout"].as<int>();
    weathercn::TIMEOUT_SEC = cfg["weathercn_timeout"].as<int>();
    
    addLog(LOG_INFO, "weather", "Config loaded");

    return 0;
}
}