add_library(mirai-api STATIC 
    util.cpp util.h 
    msg.cpp msg.h 
    http_conn.cpp http_conn.h 
    api.cpp api.h
)
target_include_directories(mirai-api
    PRIVATE ../
    PRIVATE ../thirdparty/nlohmann/json/single-include
)
set_target_properties(mirai-api PROPERTIES
    CXX_STANDARD 20)
target_link_libraries(mirai-api
    PRIVATE json
)
target_link_libraries(bbb PUBLIC mirai-api)
