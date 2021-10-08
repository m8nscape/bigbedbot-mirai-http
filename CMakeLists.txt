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
    utils/logger.cpp utils/logger.h utils/rand.h utils/strutil.cpp utils/strutil.h
    time_evt.cpp time_evt.h modules.cpp core.cpp core.h main.cpp  )
set_target_properties(bbb PROPERTIES
    CXX_STANDARD 17)

find_package(Boost REQUIRED)

add_subdirectory(thirdparty)
add_subdirectory(mirai)
add_subdirectory(app)

target_link_libraries(bbb 
    PRIVATE json
    PRIVATE yaml-cpp
    PRIVATE mirai-api
    PRIVATE app
)
target_include_directories(bbb 
    PRIVATE ${CMAKE_SOURCE_DIR}
    PRIVATE ${json_INCLUDE_DIR}
    PRIVATE ${yaml-cpp_INCLUDE_DIR}
)
find_package(CURL CONFIG)
if (CURL_FOUND)
    set(CURL_LIBRARY CURL::libcurl)
    target_link_libraries(bbb PRIVATE ${CURL_LIBRARY})
else()
    message("Using libcurl from system")
    set(CURL_LIBRARY -lcurl)
    target_link_libraries(bbb PRIVATE ${CURL_LIBRARY})
endif()

if (UNIX OR MINGW)
    target_link_libraries(bbb PRIVATE -lpthread)
    target_link_libraries(bbb PRIVATE -lstdc++fs)
endif()