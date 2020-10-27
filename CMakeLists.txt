cmake_minimum_required(VERSION 3.8...3.18)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

project(bigbetbot-mirai-http
    VERSION 1.0
    LANGUAGES CXX)
    
if (MSVC)
    add_compile_options("/utf-8")
endif()

if (WIN32)
    add_definitions(-DNOMINMAX)
endif()

add_executable(bbb 
    utils/logger.cpp utils/logger.h utils/rand.h
    time_evt.cpp time_evt.h modules.cpp core.cpp core.h main.cpp)
target_include_directories(bbb 
    PRIVATE .
    PRIVATE thirdparty/nlohmann/json/single-include
    PRIVATE thirdparty/yaml-cpp/include
)
set_target_properties(bbb PROPERTIES
    CXX_STANDARD 17)

add_subdirectory(thirdparty)
add_subdirectory(mirai)
add_subdirectory(app)

target_link_libraries(bbb 
    PRIVATE json
    PRIVATE yaml-cpp
    PRIVATE mirai-api
    PRIVATE app
)

find_package(CURL CONFIG REQUIRED)
target_link_libraries(bbb PRIVATE CURL::libcurl)
target_include_directories(bbb PRIVATE ${CURL_INCLUDE_DIR})

if (${UNIX} OR ${MINGW})
    target_link_libraries(bbb PRIVATE -lpthread)
endif()