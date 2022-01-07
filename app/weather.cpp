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

    // Do NOT verify for SSL
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

    // Setting other options for curl
    curl_buffer curlbuf;
    curl_easy_setopt(curl, CURLOPT_URL, mj_url.c_str());
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

    content = std::string(curlbuf.content, curlbuf.length); // ? might need to change .content to .text
    inQuery = false;
    return CURLE_OK;
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
    // TODO 更新代码进行关键词搜索
    addLog(LOG_DEBUG, "weather_mj", "Now Selecting ID with name: %s", name.c_str());
    auto ret = db.query("select id from cityID where dname=?", 1, { name });
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
    WEATHER_GLOBAL,
    WEATHER_CN,
    WEATHER_MJ
};
const std::vector<std::pair<std::regex, commands>> commands_regex
{
    {std::regex(R"(^weather +(.+)$)", std::regex::optimize | std::regex::extended | std::regex::icase), commands::WEATHER_GLOBAL},
    {std::regex(R"(^(.+) +weather$)", std::regex::optimize | std::regex::extended | std::regex::icase), commands::WEATHER_GLOBAL},
    {std::regex(R"(^(.+) *天气$)", std::regex::optimize | std::regex::extended), commands::WEATHER_CN},
    {std::regex(R"(^(.+) *天氣$)", std::regex::optimize | std::regex::extended), commands::WEATHER_CN},
    {std::regex(R"(^天气 +(.+)$)", std::regex::optimize | std::regex::extended), commands::WEATHER_CN},
    {std::regex(R"(^天氣 +(.+)$)", std::regex::optimize | std::regex::extended), commands::WEATHER_CN},
    {std::regex(R"(^(.+) *MJ$)", std::regex::optimize | std::regex::extended), commands::WEATHER_MJ},
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
    case commands::WEATHER_GLOBAL:
        weather_global(m, city);
        break;
    case commands::WEATHER_CN:
        weather_cn(m, city);
        break;
    case commands::WEATHER_MJ:
        weather_mj(m, city);
        break;
    default:
        break;
    }
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
void weather_mj(const mirai::MsgMetadata& m, const std::string& city_keyword)
{
    using namespace ns_weather_mj;

    // 搜索城市关键词，待更新
    auto idList = getCityId(city_keyword);
    if (idList.empty())
    {
        mirai::sendMsgRespStr(m, "[墨迹]未找到城市对应的ID");
        return;
    }

    // 遍历搜索到的list
    for (const auto& id: idList)
    {
        time_t time_req = time(NULL);
        std::string buf;
        //std::string test_str = "1000";
        
        // get real time weather by default
        // TODO get different data later.
        if (CURLE_OK != curl_post_mj(APPCODE, 6, id, buf, TIMEOUT_SEC))
        {
            // mirai::sendMsgRespStr(m, buf);
            addLog(LOG_WARNING, "weather_mj", "CURL ERROR: %d, %s", 6, id.c_str());
            continue;
        }
        
        // data is not started with '{', likely 302
        if (!buf.empty() && buf[0] != '{')
        {
            addLog(LOG_WARNING, "weather_mj", "Response json parsing error. Body: \n%s", buf.c_str());
            // mirai::sendMsgRespStr(m, "数据返回格式错误");
            continue;
        }

        // Parsing data
        nlohmann::json json = nlohmann::json::parse(buf);
        // addLog(LOG_INFO, "weather", buf.c_str());
        try
        {
            if (json.contains("data"))
            {
                // TODO 输出到ss的时候判断，删除重复的secondaryname和name（北京为直辖市）
                std::string sbuf;
                std::vector<std::string> args;

                args.push_back(json["data"]["city"]["pname"]);
                args.push_back(json["data"]["city"]["secondaryname"]);
                args.push_back(json["data"]["city"]["name"]);
                args.push_back(json["data"]["condition"]["condition"]);
                args.push_back(json["data"]["condition"]["temp"]);
                args.push_back(json["data"]["condition"]["humidity"]);
                args.push_back(json["data"]["condition"]["windDir"]);
                args.push_back(json["data"]["condition"]["windLevel"]);
                args.push_back(json["data"]["condition"]["tips"]);

                if (args[0].compare(args[1]) == 0) {args[1] = "";}
                if (args[0].compare(args[2]) == 0) {args[2] = "";}

                std::stringstream ss;
                ss << args[0] << " " << args[1] << " " << args[2] << " " << args[3] << std::endl <<
                    "实时温度：" << args[4] << "℃" << std::endl <<
                    "湿度：" << args[5] << "%" << std::endl <<
                    "风向：" << args[6] << "@" << args[7] << "级" << std::endl <<
                    "Tips：" << args[8];

                mirai::sendMsgRespStr(m, ss.str().c_str());
                
                return;
            }
            else throw std::exception();
        }
        catch (...)
        {
            addLog(LOG_WARNING, "weather", "Response parsing error. Body: \n%s", json.dump().c_str());
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