add_library(app STATIC 
    common/dbconn.cpp common/dbconn.h
    data/group.cpp data/group.h data/user.cpp data/user.h
    case.cpp case.h
    help.cpp help.h
    tools.cpp tools.h
)
target_include_directories(app 
    PRIVATE ../
)
set_target_properties(app PROPERTIES
    CXX_STANDARD 17)
target_link_libraries(app 
    PRIVATE mirai-api
    PRIVATE json
    PRIVATE yaml-cpp
)
target_link_libraries(bbb PUBLIC app)
