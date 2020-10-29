cmake_minimum_required(VERSION 3.8...3.18)

add_library(mirai-api STATIC 
    util.cpp util.h 
    msg.cpp msg.h 
    http_conn.cpp http_conn.h 
    api.cpp api.h
)
target_include_directories(mirai-api
    PRIVATE ${CMAKE_SOURCE_DIR}
)
set_target_properties(mirai-api PROPERTIES
    CXX_STANDARD 17)
target_link_libraries(mirai-api
    PRIVATE json
)

target_link_libraries(mirai-api PRIVATE Boost::disable_autolinking)