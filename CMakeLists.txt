cmake_minimum_required(VERSION 3.8...3.18)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

project(bigbetbot-mirai-http
    VERSION 1.0
    LANGUAGES CXX)

add_executable(bbb logger.cpp logger.h time_evt.cpp time_evt.h core.cpp core.h main.cpp)
target_include_directories(bbb 
    PRIVATE .
    PRIVATE thirdparty
)
set_target_properties(bbb PROPERTIES
    CXX_STANDARD 20)

add_subdirectory(thirdparty)
add_subdirectory(mirai)
add_subdirectory(app)

target_link_libraries(bbb 
    PRIVATE -lsqlite3
    PRIVATE -lcurl
    PRIVATE -lpthread
    PUBLIC yaml-cpp
)