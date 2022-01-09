#include <vector>
#include <string>
#include <sstream>
#include <regex>

#if __GNUC__ >= 8
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

#include "weather.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

#include "common/dbconn.h"
#include "utils/logger.h"
#include "utils/strutil.h"
#include "mirai/api.h"
#include "mirai/msg.h"

namespace weather
{

// %%%%%%%%%%%%%%% CURL %%%%%%%%%%%%%%%

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

    int ret = curl_easy_perform(curl);
    if (CURLE_OK == ret)
    {
        content = std::string(curlbuf.content, curlbuf.length);
    }
    else
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
    }

    inQuery = false;
    curl_easy_cleanup(curl);
    return CURLE_OK;
}

int curl_post_mj(const std::string& appcode, const int search_type, const std::string& city_id, std::string& content, int timeout_sec = 5)
{
    // Search type
    // 0    限行数据
    // 1    空气质量指数
    // 2    生活指数
    // 3    天气预警
    // 4    天气预报24小时
    // 5    天气预报15天
    // 6    天气实况
    // 7    AQI预报5天
    if (inQuery)
    {
        content = "API is busy";
        return -2;
    }

    inQuery = true;

    // check if search_type is OOB
    if (search_type < 0 || search_type > 7)
    {
        content = "search_type out of bound";
        return -3;
    }

    // Place Moji Tokens here for test
    std::vector<std::string> mj_token = 
    {
        "27200005b3475f8b0e26428f9bfb13e9",
        "8b36edf8e3444047812be3a59d27bab9",
        "5944a84ec4a071359cc4f6928b797f91",
        "7ebe966ee2e04bbd8cdbc0b84f7f3bc7",
        "008d2ad9197090c5dddc76f583616606",
        "f9f212e1996e79e0e602b08ea297ffb0",
        "50b53ff8dd7d9fa320d3d3ca32cf8ed1",
        "0418c1f4e5e66405d33556418189d2d0"
    };

    std::vector<std::string> mj_url_append = 
    {
        "limit", "aqi", "index", "alert",
        "forecast24hours", "forecast15days", "condition", "aqiforecast5days"
    };

    std::string mj_url = "https://aliv18.mojicb.com/whapi/json/alicityweather/";
    mj_url += mj_url_append[search_type];

    // Initializing CURL
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        content = "curl init failed";
        inQuery = false;
        return -1;
    }

    // HTTP HEADERS, remember to replace <MJ_APPCODE> with real appcode
    curl_slist *plist = NULL;

    std::string arg_app = "appcode: " + appcode;
    std::string arg_auth = "Authorization: APPCODE " + appcode;
    plist = curl_slist_append(plist, arg_app.c_str());
    plist = curl_slist_append(plist, "Content-Type: application/x-www-form-urlencoded; charset=UTF-8"); 
    plist = curl_slist_append(plist, arg_auth.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, plist);

    // POST body
    std::string strJsonData = strfmt("cityId=%s&token=%s", city_id.c_str(), mj_token[search_type].c_str());

    // Setting POST variables for curl
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strJsonData.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strJsonData.size());

    // verify SSL
#ifdef __linux__
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_CAPATH, "/etc/ssl/certs");
#else
    // not sure about other systems
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

    // Setting other options for curl
    curl_buffer curlbuf;
    curl_easy_setopt(curl, CURLOPT_URL, mj_url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, perform_write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curlbuf);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_sec);
    memset(curlbuf.content, 0, CURL_MAX_WRITE_SIZE);

    int ret = curl_easy_perform(curl);
    if (CURLE_OK == ret)
    {
        content = std::string(curlbuf.content, curlbuf.length); // ? might need to change .content to .text
        inQuery = false;
    }
    else
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
    }

    inQuery = false;
    curl_slist_free_all(plist);
    curl_easy_cleanup(curl);
    return ret;
}

// %%%%%%%%%%%%%%% API Specific Function %%%%%%%%%%%%%%%

namespace weathercn
{
int TIMEOUT_SEC = 5;
SQLite db("weathercn_city.db", "weather");

std::vector<std::string> getCityId(const std::string& name)
{
    // I would like to use LIKE phrase but SQL injection should be considered
    auto ret = db.query("select id from cityid where name=?", 1, { name });
    if (ret.empty()) return {};
    std::vector<std::string> idList;
    for (auto& r: ret)
        idList.push_back(std::any_cast<std::string>(r[0]));
    return idList;
}

// https://www.sojson.com/blog/305.html
std::string getReqUrl(const std::string& id)
{
    std::string ret = strfmt(
        "http://t.weather.itboy.net/api/weather/city/%s",
        id.c_str());
    return ret;
}

// 1.接口每8小时更新一次，机制是CDN缓存8小时更新一次。
// ! 注意 !
// ! 自己做缓存，因为你每请求我一次，我就是有费用的，又拍云CDN加速回源是按次收费，你可以了解下
std::map<std::string, std::pair<time_t, std::string>> api_buf;
const time_t BUF_VALID_DURATION = 8 * 60 * 60;
}

namespace openweather
{
std::string APIKEY;
int TIMEOUT_SEC = 5;
std::string getReqUrl(const std::string& name)
{
    std::string ret = strfmt(
        "https://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s",
        name.c_str(),
        APIKEY.c_str());
    return ret;
}
}

namespace ns_weather_mj
{
std::string APPCODE;
int TIMEOUT_SEC = 5;
SQLite db("weatherMJ_cityID.db", "weather_mj");

// [墨迹天气] 输入城市名，获取城市ID
std::vector<std::string> getCityId(const std::string& name)
{
    // Using LIKE as the database files are seperated and the cityID table does not contain sensitive data
    auto ret = db.query("select id from cityID where dname like ?", 1, { '%' + name + '%' });
    if (ret.empty()) return {};
    std::vector<std::string> idList;
    for (auto& r: ret)
        idList.push_back(std::any_cast<std::string>(r[0]));
    return idList;
}
}

// %%%%%%%%%%%%%% BUILD RES MSG %%%%%%%%%%%%%%

// %%%%%%%%%%%%%% COMMANDS %%%%%%%%%%%%%%

enum class commands : size_t {
    _,
    WEATHER_HELP,
    WEATHER_GLOBAL,
    WEATHER_CN,
    WEATHER_MJ_REALTIME,
    WEATHER_MJ_AQI,
    WEATHER_MJ_LIFE,
    WEATHER_MJ_WARNING,
    WEATHER_MJ_FORECAST24H,
    WEATHER_MJ_FORECAST15D,
    WEATHER_MJ_AQI_FORECAST
};
const std::vector<std::pair<std::regex, commands>> commands_regex
{
    {std::regex(R"(^天气帮助$)", std::regex::optimize | std::regex::extended), commands::WEATHER_HELP},
    
    {std::regex(R"(^weather +(.+)$)", std::regex::optimize | std::regex::extended | std::regex::icase), commands::WEATHER_GLOBAL},
    {std::regex(R"(^(.+) +weather$)", std::regex::optimize | std::regex::extended | std::regex::icase), commands::WEATHER_GLOBAL},

    {std::regex(R"(^(.+) *天气$)", std::regex::optimize | std::regex::extended), commands::WEATHER_CN},
    {std::regex(R"(^(.+) *天氣$)", std::regex::optimize | std::regex::extended), commands::WEATHER_CN},
    {std::regex(R"(^天气 +(.+)$)", std::regex::optimize | std::regex::extended), commands::WEATHER_CN},
    {std::regex(R"(^天氣 +(.+)$)", std::regex::optimize | std::regex::extended), commands::WEATHER_CN},

    {std::regex(R"(^(.+) *实时$)", std::regex::optimize | std::regex::extended), commands::WEATHER_MJ_REALTIME},
    {std::regex(R"(^实时 +(.+)$)", std::regex::optimize | std::regex::extended), commands::WEATHER_MJ_REALTIME},
    {std::regex(R"(^(.+) *实时天气$)", std::regex::optimize | std::regex::extended), commands::WEATHER_MJ_REALTIME},
    {std::regex(R"(^实时天气 +(.+)$)", std::regex::optimize | std::regex::extended), commands::WEATHER_MJ_REALTIME},
    {std::regex(R"(^(.+) *實時$)", std::regex::optimize | std::regex::extended), commands::WEATHER_MJ_REALTIME},
    {std::regex(R"(^實時 +(.+)$)", std::regex::optimize | std::regex::extended), commands::WEATHER_MJ_REALTIME},
    {std::regex(R"(^(.+) *實時天氣$)", std::regex::optimize | std::regex::extended), commands::WEATHER_MJ_REALTIME},
    {std::regex(R"(^實時天氣 +(.+)$)", std::regex::optimize | std::regex::extended), commands::WEATHER_MJ_REALTIME},

    {std::regex(R"(^(.+) *空气成分$)", std::regex::optimize | std::regex::extended), commands::WEATHER_MJ_AQI},
    {std::regex(R"(^空气成分 +(.+)$)", std::regex::optimize | std::regex::extended), commands::WEATHER_MJ_AQI},

    {std::regex(R"(^(.+) *生活指数$)", std::regex::optimize | std::regex::extended), commands::WEATHER_MJ_LIFE},
    {std::regex(R"(^生活指数 +(.+)$)", std::regex::optimize | std::regex::extended), commands::WEATHER_MJ_LIFE},

    {std::regex(R"(^(.+) *天气预警$)", std::regex::optimize | std::regex::extended), commands::WEATHER_MJ_WARNING},
    {std::regex(R"(^天气预警 +(.+)$)", std::regex::optimize | std::regex::extended), commands::WEATHER_MJ_WARNING},

    {std::regex(R"(^(.+) *天气预报$)", std::regex::optimize | std::regex::extended), commands::WEATHER_MJ_FORECAST24H},
    {std::regex(R"(^天气预报 +(.+)$)", std::regex::optimize | std::regex::extended), commands::WEATHER_MJ_FORECAST24H},

    {std::regex(R"(^(.+) *一周预报$)", std::regex::optimize | std::regex::extended), commands::WEATHER_MJ_FORECAST15D},
    {std::regex(R"(^一周预报 +(.+)$)", std::regex::optimize | std::regex::extended), commands::WEATHER_MJ_FORECAST15D},

    {std::regex(R"(^(.+) *空气预报$)", std::regex::optimize | std::regex::extended), commands::WEATHER_MJ_AQI_FORECAST},
    {std::regex(R"(^空气预报 +(.+)$)", std::regex::optimize | std::regex::extended), commands::WEATHER_MJ_AQI_FORECAST},
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
    case commands::WEATHER_HELP:
        weather_help(m);
        break;
    case commands::WEATHER_GLOBAL:
        weather_global(m, city);
        break;
    case commands::WEATHER_CN:
        weather_cn(m, city);
        break;
    case commands::WEATHER_MJ_AQI:
        weather_mj(m, city, 1);
        break;
    case commands::WEATHER_MJ_LIFE:
        weather_mj(m, city, 2);
        break;
    case commands::WEATHER_MJ_WARNING:
        weather_mj(m, city, 3);
        break;
    case commands::WEATHER_MJ_FORECAST24H:
        weather_mj(m, city, 4);
        break;
    case commands::WEATHER_MJ_FORECAST15D:
        weather_mj(m, city, 5);
        break;
    case commands::WEATHER_MJ_REALTIME:
        weather_mj(m, city, 6);
        break;
    case commands::WEATHER_MJ_AQI_FORECAST:
        weather_mj(m, city, 7);
        break;
    default:
        break;
    }
}

// [天气帮助] Get help message for weather commands
void weather_help(const mirai::MsgMetadata& m)
{
    addLog(LOG_INFO, "weather", "Printing Help message for Weather module");
    std::stringstream ss;
        ss << "目前的天气指令有：" << std::endl
           << "天气帮助         返回此帮助" << std::endl
           << "[城市]天气       查询中国城市天气" << std::endl
           << "[城市]weather    查询世界城市天气" << std::endl
           << "[城市]实时       查询实时天气" << std::endl
           << "[城市]空气成分   查询城市的空气污染物组成(CO2等)" << std::endl
           << "[城市]生活指数   查询生活指数(感冒指数、洗车钓鱼)" << std::endl
           << "[城市]天气预警   查询气象预警信息" << std::endl
           << "[城市]天气预报   查询24小时天气预报" << std::endl
           << "[城市]一周天气   [暂不可用]查询15天(一周)天气预报" << std::endl
           << "[城市]空气预报   [暂不可用]查询5天AQI预报" << std::endl;
    mirai::sendMsgRespStr(m, ss.str().c_str());
    return;
}

// [全球天气] Construct bot reply message for weather_global from city name
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

// [中国天气] Construct bot reply message for weather_cn from city name
void weather_cn(const mirai::MsgMetadata& m, const std::string& name)
{
    using namespace weathercn;

    auto idList = getCityId(name);
    if (idList.empty())
    {
        mirai::sendMsgRespStr(m, "未找到城市对应的ID");
        return;
    }
    for (const auto& id: idList)
    {
        // get data from buffer if not expired
        time_t time_req = time(NULL);
        std::string buf;
        if (api_buf.find(id) != api_buf.end())
        {
            auto [time_buf, data] = api_buf.at(id);
            if (time_req - time_buf < BUF_VALID_DURATION)
            {
                mirai::sendMsgRespStr(m, data.c_str());
                return;
            }
        }
        
        auto url = getReqUrl(id);
        if (CURLE_OK != curl_get(url, buf, TIMEOUT_SEC))
        {
            //mirai::sendMsgRespStr(m, buf);
            addLog(LOG_WARNING, "weather", "CURL ERROR: %s", url.c_str());
            continue;
        }
        
        // data is not started with '{', likely 302
        if (!buf.empty() && buf[0] != '{')
        {
            addLog(LOG_WARNING, "weather", "Response json parsing error. Body: \n%s", buf.c_str());
            // mirai::sendMsgRespStr(m, "数据返回格式错误");
            continue;
        }

        // parse data
        nlohmann::json json = nlohmann::json::parse(buf);
        // addLog(LOG_INFO, "weather", buf.c_str());
        try
        {
            if (json.contains("cityInfo"))
            {
                int ibuf;
                double dbuf;
                std::string sbuf;
                std::vector<std::string> args;

                args.push_back(json["cityInfo"]["parent"]);
                args.push_back(json["cityInfo"]["city"]);
                args.push_back(json["data"]["wendu"]);
                args.push_back(json["data"]["shidu"]);

                dbuf = json["data"]["pm25"];
                sbuf = strfmt("%.1f", dbuf);
                args.push_back(sbuf);

                dbuf = json["data"]["pm10"];
                sbuf = strfmt("%.1f", dbuf);
                args.push_back(sbuf);

                args.push_back(json["data"]["forecast"][0]["type"]);
                args.push_back(json["data"]["forecast"][0]["low"]);
                args.push_back(json["data"]["forecast"][0]["high"]);

                ibuf = json["data"]["forecast"][0]["aqi"];
                sbuf = strfmt("%d", ibuf);
                args.push_back(sbuf);

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
                
                // save to buffer
                api_buf[id] = {time_req, ss.str()};
                return;
            }
            else throw std::exception();
        }
        catch (...)
        {
            addLog(LOG_WARNING, "weather", "Response parsing error. Body: \n%s", json.dump().c_str());
            //mirai::sendMsgRespStr(m, "天气解析失败");
        }
    }

    addLog(LOG_WARNING, "weather", "No valid request for %s", name.c_str());
    mirai::sendMsgRespStr(m, "天气解析失败");
}

// [墨迹天气] Construct bot reply message for moji weather from location keyword
void weather_mj(const mirai::MsgMetadata& m, const std::string& city_keyword, const int search_type)
{
    using namespace ns_weather_mj;

    // 搜索城市关键词
    auto idList = getCityId(city_keyword);
    if (idList.empty())
    {
        mirai::sendMsgRespStr(m, "[墨迹]未找到城市对应的ID");
        return;
    }

    // 遍历搜索到的list(仅搜索一项)
    for (const auto& id: idList)
    {
        time_t time_req = time(NULL);

        switch(search_type)
        {
        case 1: // AQI
        {
            std::string buf;
            
            // Get Air Quality Data
            if (CURLE_OK != curl_post_mj(APPCODE, search_type, id, buf, TIMEOUT_SEC))
            {
                addLog(LOG_WARNING, "weather_mj", "CURL ERROR: %d, %s", search_type, id.c_str());
                continue;
            }
            
            // data is not started with '{', likely 302
            if (buf.empty())
            {
                addLog(LOG_WARNING, "weather_mj", "Response is empty. id:%s Body: \n", id.c_str());
                continue;
            }
            else if (buf[0] != '{')
            {
                addLog(LOG_WARNING, "weather_mj", "Response json parsing error. id:%s Body: \n%s", id.c_str(), buf.c_str());
                continue;
            }

            // Parsing data
            nlohmann::json json = nlohmann::json::parse(buf);

            try
            {
                if (json.contains("data"))
                {
                    std::vector<std::string> args;

                    args.push_back(json["data"]["aqi"]["cityName"]); // args[0]
                    args.push_back(json["data"]["aqi"]["co"]);       // args[1]
                    args.push_back(json["data"]["aqi"]["no2"]);      // args[2]
                    args.push_back(json["data"]["aqi"]["o3"]);       // args[3]
                    args.push_back(json["data"]["aqi"]["pm10"]);     // args[4]
                    args.push_back(json["data"]["aqi"]["pm25"]);     // args[5]
                    args.push_back(json["data"]["aqi"]["so2"]);      // args[6]
                    args.push_back(json["data"]["aqi"]["value"]);    // args[7]
                    args.push_back(json["data"]["aqi"]["rank"]);     // args[8]

                    std::stringstream ss;
                    ss << args[0] << std::endl <<
                        "一氧化碳指数: " << args[1] << std::endl <<
                        "二氧化氮指数: " << args[2] << std::endl <<
                        "臭氧指数: "     << args[3] << std::endl <<
                        "PM10指数: "     << args[4] << std::endl <<
                        "PM2.5指数: "    << args[5] << std::endl <<
                        "二氧化硫指数: " << args[6] << std::endl <<
                        "AQI: " << args[7];
                    
                    int aqi = atoi(args[7].c_str());
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
                    
                    ss << std::endl <<
                        "空气质量排行: " << args[8];

                    mirai::sendMsgRespStr(m, ss.str().c_str());
                    
                    return;
                }
                else throw std::exception();
            }
            catch (...)
            {
                addLog(LOG_WARNING, "weather_mj", "Response parsing error. Body: \n%s", json.dump().c_str());
            }
            break;
        }
        case 2: // 生活指数
        {
            std::string buf;
            
            // Get Life Data
            if (CURLE_OK != curl_post_mj(APPCODE, search_type, id, buf, TIMEOUT_SEC))
            {
                addLog(LOG_WARNING, "weather_mj", "CURL ERROR: %d, %s", 6, id.c_str());
                continue;
            }
            
            // data is not started with '{', likely 302
            if (buf.empty())
            {
                addLog(LOG_WARNING, "weather_mj", "Response is empty. id:%s Body: \n", id.c_str());
                continue;
            }
            else if (buf[0] != '{')
            {
                addLog(LOG_WARNING, "weather_mj", "Response json parsing error. id:%s Body: \n%s", id.c_str(), buf.c_str());
                continue;
            }

            // Parsing data
            nlohmann::json json = nlohmann::json::parse(buf);

            try
            {
                if (json.contains("data"))
                {
                    std::vector<std::string> args;

                    args.push_back(json["data"]["city"]["pname"]);          // args[0]
                    args.push_back(json["data"]["city"]["secondaryname"]);  // args[1]
                    args.push_back(json["data"]["city"]["name"]);           // args[2]
                    
                    // TODO get current day in china
                    // TODO write for loop to get live index and print them

                    // 删除重复的secondaryname和name（例如：北京为直辖市，输出不应为北京市北京市）
                    if (args[0].compare(args[1]) == 0) args[1] = "";
                    if (args[0].compare(args[2]) == 0) args[2] = "";

                    std::stringstream ss;
                    ss << args[0] << " " << args[1] << " " << args[2] << std::endl <<
                        "在写了.jpg";

                    mirai::sendMsgRespStr(m, ss.str().c_str());
                    
                    return;
                }
                else throw std::exception();
            }
            catch (...)
            {
                addLog(LOG_WARNING, "weather_mj", "Response parsing error. Body: \n%s", json.dump().c_str());
            }
            break;
        }
        case 3: // 预警
        {
            std::string buf;
            
            // Get Warning
            if (CURLE_OK != curl_post_mj(APPCODE, search_type, id, buf, TIMEOUT_SEC))
            {
                addLog(LOG_WARNING, "weather_mj", "CURL ERROR: %d, %s", search_type, id.c_str());
                continue;
            }
            
            // data is not started with '{', likely 302
            if (buf.empty())
            {
                addLog(LOG_WARNING, "weather_mj", "Response is empty. id:%s Body: \n", id.c_str());
                continue;
            }
            else if (buf[0] != '{')
            {
                addLog(LOG_WARNING, "weather_mj", "Response json parsing error. id:%s Body: \n%s", id.c_str(), buf.c_str());
                continue;
            }

            // Parsing data
            nlohmann::json json = nlohmann::json::parse(buf);

            try
            {
                if (json.contains("data"))
                {
                    std::vector<std::string> args;

                    args.push_back(json["data"]["alert"][0]["title"]);   // args[0]
                    args.push_back(json["data"]["alert"][0]["content"]); // args[1]

                    std::stringstream ss;
                    ss << args[0] << std::endl
                       << args[1] << std::endl;

                    mirai::sendMsgRespStr(m, ss.str().c_str());
                    
                    return;
                }
                else throw std::exception();
            }
            catch (...)
            {
                addLog(LOG_WARNING, "weather_mj", "Response parsing error. Body: \n%s", json.dump().c_str());
            }
            break;
        }
        case 4: // 24小时预报
        {
            std::string buf;
            
            // Get 24h forecast
            if (CURLE_OK != curl_post_mj(APPCODE, search_type, id, buf, TIMEOUT_SEC))
            {
                addLog(LOG_WARNING, "weather_mj", "CURL ERROR: %d, %s", search_type, id.c_str());
                continue;
            }
            
            // data is not started with '{', likely 302
            if (buf.empty())
            {
                addLog(LOG_WARNING, "weather_mj", "Response is empty. id:%s Body: \n", id.c_str());
                continue;
            }
            else if (buf[0] != '{')
            {
                addLog(LOG_WARNING, "weather_mj", "Response json parsing error. id:%s Body: \n%s", id.c_str(), buf.c_str());
                continue;
            }

            // Parsing data
            nlohmann::json json = nlohmann::json::parse(buf);

            try
            {
                if (json.contains("data"))
                {
                    std::vector<std::string> args;
                    std::vector<int> args_temp;
                    std::vector<int> args_realFeel;

                    args.push_back(json["data"]["city"]["pname"]);          // args[0]
                    args.push_back(json["data"]["city"]["secondaryname"]);  // args[1]
                    args.push_back(json["data"]["city"]["name"]);           // args[2]

                    // 删除重复的secondaryname和name（例如：北京为直辖市，输出不应为北京市北京市）
                    if (args[0].compare(args[1]) == 0) args[1] = "";
                    if (args[0].compare(args[2]) == 0) args[2] = "";

                    std::stringstream ss;
                    ss << args[0] << " " << args[1] << " " << args[2] << std::endl;

                    for (int i = 0; i < 24; ++i) // sorry there should be a better way, use this for early testing
                    {
                        args_temp.push_back(std::atoi(json["data"]["hourly"][i]["temp"].get<std::string>().c_str()));
                        args_realFeel.push_back(std::atoi(json["data"]["hourly"][i]["realFeel"].get<std::string>().c_str()));
                    }

                    int max_temp = *max_element(std::begin(args_temp), std::end(args_temp));
                    int min_temp = *min_element(std::begin(args_temp), std::end(args_temp));
                    int max_real = *max_element(std::begin(args_realFeel), std::end(args_realFeel));
                    int min_real = *min_element(std::begin(args_realFeel), std::end(args_realFeel));

                    ss << "未来24小时，温度最低" << std::to_string(min_temp) << "°C，最高" << std::to_string(max_temp) << "°C" << std::endl
                       << "体感温度最低" << std::to_string(min_real) << "°C，最高" << std::to_string(max_real) << "°C";

                    // 未来24小时，温度最低[]°C，最高[]°C
                    // 体感温度最低[]°C，最高[]°C
                    // ? 是否可以增加一些其他信息？

                    mirai::sendMsgRespStr(m, ss.str().c_str());
                    
                    return;
                }
                else throw std::exception();
            }
            catch (...)
            {
                addLog(LOG_WARNING, "weather_mj", "Response parsing error. Body: \n%s", json.dump().c_str());
            }
            break;
        }
        case 5: // 15天预报
        {
            std::stringstream ss;
            ss << "OMG你查这个干啥呢";

            mirai::sendMsgRespStr(m, ss.str().c_str());
            return;
            break;
        }
        case 6: // 实时
        {
            std::string buf;
            std::string buf_aqi;
            
            // get real time weather
            if (CURLE_OK != curl_post_mj(APPCODE, 6, id, buf, TIMEOUT_SEC))
            {
                addLog(LOG_WARNING, "weather_mj", "CURL ERROR: %d, %s", 6, id.c_str());
                continue;
            }

            // 获取AQI数据
            if (CURLE_OK != curl_post_mj(APPCODE, 1, id, buf_aqi, TIMEOUT_SEC))
            {
                addLog(LOG_WARNING, "weather_mj", "CURL ERROR: %d, %s", 1, id.c_str());
                continue;
            }
            
            // data is not started with '{', likely 302
            if (buf.empty() || buf_aqi.empty())
            {
                addLog(LOG_WARNING, "weather_mj", "Response is empty. id:%s Body: \n", id.c_str());
                continue;
            }
            else if (buf[0] != '{' || buf_aqi[0] != '{')
            {
                addLog(LOG_WARNING, "weather_mj", "Response json parsing error. id:%s Body: \n%s", id.c_str(), buf.c_str());
                continue;
            }

            // Parsing data
            nlohmann::json json = nlohmann::json::parse(buf);
            nlohmann::json json_aqi = nlohmann::json::parse(buf_aqi);

            try
            {
                if (json.contains("data"))
                {
                    std::string sbuf;
                    std::vector<std::string> args;

                    args.push_back(json["data"]["city"]["pname"]);          // args[0]
                    args.push_back(json["data"]["city"]["secondaryname"]);  // args[1]
                    args.push_back(json["data"]["city"]["name"]);           // args[2]
                    args.push_back(json["data"]["condition"]["condition"]); // args[3]
                    args.push_back(json["data"]["condition"]["temp"]);      // args[4]
                    args.push_back(json["data"]["condition"]["humidity"]);  // args[5]
                    args.push_back(json["data"]["condition"]["windDir"]);   // args[6]
                    args.push_back(json["data"]["condition"]["windLevel"]); // args[7]
                    args.push_back(json["data"]["condition"]["tips"]);      // args[8]
                    args.push_back(json_aqi["data"]["aqi"]["value"]);       // args[9]
                    args.push_back(json_aqi["data"]["aqi"]["rank"]);        // args[10]

                    // 删除重复的secondaryname和name（例如：北京为直辖市，输出不应为北京市北京市）
                    if (args[0].compare(args[1]) == 0) args[1] = "";
                    if (args[0].compare(args[2]) == 0) args[2] = "";

                    std::stringstream ss;
                    ss << args[0] << " " << args[1] << " " << args[2] << " " << args[3] << std::endl <<
                        "实时温度：" << args[4] << "℃" << std::endl <<
                        "湿度：" << args[5] << "%" << std::endl <<
                        "风向：" << args[6] << "@" << args[7] << "级" << std::endl <<
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
                    
                    ss << std::endl <<
                        "空气质量排行: " << args[10];

                    mirai::sendMsgRespStr(m, ss.str().c_str());
                    
                    return;
                }
                else throw std::exception();
            }
            catch (...)
            {
                addLog(LOG_WARNING, "weather", "Response parsing error. Body: \n%s", json.dump().c_str());
            }
            break;
        }
        case 7: // 5天AQI预报
        {
            std::stringstream ss;
            ss << "说真的这个也没用";

            mirai::sendMsgRespStr(m, ss.str().c_str());
            return;
            break;
        }
        default:
            break;
        }
    }

    addLog(LOG_WARNING, "weather", "No valid request for %s", city_keyword.c_str());
    mirai::sendMsgRespStr(m, "天气解析失败");
}

int init(const char* yaml)
{
    fs::path cfgPath(yaml);
    if (!fs::is_regular_file(cfgPath))
    {
        addLog(LOG_ERROR, "weather", "weather config file %s not found", fs::absolute(cfgPath).c_str());
        return -1;
    }
    addLog(LOG_INFO, "weather", "Loading weather config from %s", fs::absolute(cfgPath).c_str());

    YAML::Node cfg = YAML::LoadFile(yaml);

    // 配置
    openweather::APIKEY = cfg["openweather_apikey"].as<std::string>();
    openweather::TIMEOUT_SEC = cfg["openweather_timeout"].as<int>();
    weathercn::TIMEOUT_SEC = cfg["weathercn_timeout"].as<int>();
    ns_weather_mj::APPCODE = cfg["mjweather_appcode"].as<std::string>();
    ns_weather_mj::TIMEOUT_SEC = cfg["mjweather_timeout"].as<int>();
    
    addLog(LOG_INFO, "weather", "Config loaded");

    return 0;
}
}