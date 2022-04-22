#include "api.h"
#include "../data/user.h"
#include "../data/group.h"
#include "../smoke.h"

namespace bbb::api::user
{
using nlohmann::json;

ProcResp get_info(std::string_view path, const std::map<std::string, std::string> params, const std::string& body)
{
    if (params.find("qq") == params.end())
        return {beast::http::status::ok, R"({"result": 1, "msg": "param 'qq' not found"})"};

    int64_t qq = std::stoll(params.at("qq"));

    if (::user::plist.find(qq) == ::user::plist.end())
    {
        char buf[64];
        sprintf(buf, R"({"result": 1, "msg": "user '%ld' not found"})", qq);
        return { beast::http::status::ok, buf };
    }

    json j, jd;
    ::user::pdata& data = ::user::plist[qq];
    j["result"] = 0;
    j["msg"] = "success";
    auto s = data.getStamina(false);
    jd["qq"] = qq;
    jd["currency"] = data.getCurrency();
    jd["key"] = data.getKeyCount();
    jd["stamina"] = s.staminaAfterUpdate;
    jd["extra_stamina"] = data.getExtraStamina();
    jd["stamina_recovery_time"] = s.fullRecoverTime;
    j["data"] = jd;
    return { beast::http::status::ok, j.dump() };
}

ProcResp get_group_info(std::string_view path, const std::map<std::string, std::string> params, const std::string& body)
{
    if (params.find("qq") == params.end())
        return {beast::http::status::ok, R"({"result": 1, "msg": "param 'qq' not found"})"};
    if (params.find("group") == params.end())
        return {beast::http::status::ok, R"({"result": 1, "msg": "param 'group' not found"})"};

    int64_t qq = std::stoll(params.at("qq"));
    int64_t group = std::stoll(params.at("group"));

    if (::user::plist.find(qq) == ::user::plist.end())
    {
        char buf[64];
        sprintf(buf, R"({"result": 1, "msg": "user '%ld' not found"})", qq);
        return { beast::http::status::ok, buf };
    }

    if (::grp::groups.find(group) == ::grp::groups.end())
    {
        char buf[80];
        sprintf(buf, R"({"result": 1, "msg": "group '%ld' not found"})", group);
        return { beast::http::status::ok, buf };
    }

    json j, jd;
    ::user::pdata& data = ::user::plist[qq];
    j["result"] = 0;
    j["msg"] = "success";
    jd["qq"] = qq;
    jd["group"] = group;
    jd["silenced"] = ::smoke::isSmoking(qq, group);
    j["data"] = jd;
    return { beast::http::status::ok, j.dump() };
}

ProcResp post_give_currency(std::string_view path, const std::map<std::string, std::string> params, const std::string& body)
{
    if (params.find("qq") == params.end())
        return {beast::http::status::ok, R"({"result": 1, "msg": "param 'qq' not found"})"};
    if (params.find("amount") == params.end())
        return {beast::http::status::ok, R"({"result": 1, "msg": "param 'amount' not found"})"};
    
    int64_t qq = std::stoll(params.at("qq"));
    int64_t amount = std::stoll(params.at("amount"));

    if (::user::plist.find(qq) == ::user::plist.end())
    {
        char buf[64];
        sprintf(buf, R"({"result": 1, "msg": "user '%ld' not found"})", qq);
        return { beast::http::status::ok, buf };
    }

    if (amount <= 0)
    {
        return {beast::http::status::ok, R"({"result": 1, "msg": "param 'amount' should be above zero"})"};
    }

    ::user::pdata& data = ::user::plist[qq];
    data.modifyCurrency(+amount);
    int64_t currency = data.getCurrency();

    json j, jd;
    j["result"] = 0;
    j["msg"] = "success";
    jd["qq"] = qq;
    jd["currency"] = currency;
    j["data"] = jd;
    return { beast::http::status::ok, j.dump() };
}

ProcResp post_take_currency(std::string_view path, const std::map<std::string, std::string> params, const std::string& body)
{

    if (params.find("qq") == params.end())
        return {beast::http::status::ok, R"({"result": 1, "msg": "param 'qq' not found"})"};
    if (params.find("amount") == params.end())
        return {beast::http::status::ok, R"({"result": 1, "msg": "param 'amount' not found"})"};
    
    int64_t qq = std::stoll(params.at("qq"));
    int64_t amount = std::stoll(params.at("amount"));

    if (::user::plist.find(qq) == ::user::plist.end())
    {
        char buf[64];
        sprintf(buf, R"({"result": 1, "msg": "user '%ld' not found"})", qq);
        return { beast::http::status::ok, buf };
    }

    if (amount >= 0)
    {
        return {beast::http::status::ok, R"({"result": 1, "msg": "param 'amount' should be below zero"})"};
    }

    ::user::pdata& data = ::user::plist[qq];
    data.modifyCurrency(-amount);
    int64_t currency = data.getCurrency();

    json j, jd;
    j["result"] = 0;
    j["msg"] = "success";
    jd["qq"] = qq;
    jd["currency"] = currency;
    j["data"] = jd;
    return { beast::http::status::ok, j.dump() };
}

}