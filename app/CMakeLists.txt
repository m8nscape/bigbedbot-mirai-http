add_library(app STATIC 
    common/dbconn.cpp common/dbconn.h
    data/group.cpp data/group.h data/user.cpp data/user.h
    case.cpp case.h
    help.cpp help.h
    tools.cpp tools.h
)
target_include_directories(app 
    PRIVATE ../
    PRIVATE ../thirdparty
)
set_target_properties(app PROPERTIES
    CXX_STANDARD 20)
target_link_libraries(app 
    PRIVATE mirai-api
    PRIVATE yaml-cpp
)
target_link_libraries(bbb PUBLIC app)
