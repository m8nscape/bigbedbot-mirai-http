#include "apievent.h"
#include "data/group.h"
#include "data/user.h"
#include <sstream>

// https://github.com/project-mirai/mirai-api-http/blob/master/docs/api/EventType.md

namespace apievent
{
void NewFriendRequestEvent(const json& req)
{
    int64_t reqGroupId = req.at("groupId");
    int64_t reqQQId = req.at("fromId");
    addLog(LOG_INFO, "apievent", "Received NewFriendRequestEvent from %lld @ %lld", reqQQId, reqGroupId);
    if (reqQQId == rootQQId || 
        (grp::groups.find(reqGroupId) != grp::groups.end() && user::plist.find(reqQQId) != user::plist.end()))
    {
        mirai::respNewFriendRequestEvent(req, 0,);
        // TODO help message?
    }
    else if (rootQQId > 0)
    {
        std::stringstream ss;
        ss << "有陌生人加好友 QQ:" << reqQQId << " Group:" << reqGroupId;
        mirai::sendFriendMsgStr(rootQQId, ss.str());
    }
}

}