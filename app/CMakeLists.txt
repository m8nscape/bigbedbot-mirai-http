cmake_minimum_required(VERSION 3.8...3.18)

add_library(app STATIC 
    common/dbconn.cpp common/dbconn.h
    data/group.cpp data/group.h data/user.cpp data/user.h
    case.cpp case.h
    help.cpp help.h
    tools.cpp tools.h
    smoke.cpp smoke.h
)
target_include_directories(app 
    PRIVATE ../
    PRIVATE ../thirdparty/nlohmann/json/single-include
    PRIVATE ../thirdparty/yaml-cpp/include
)
set_target_properties(app PROPERTIES
    CXX_STANDARD 17)
target_link_libraries(app 
    PRIVATE mirai-api
    PRIVATE json
    PRIVATE yaml-cpp
)

find_package(SQLite3 CONFIG)
if (SQLite3_FOUND)
    target_link_libraries(app PRIVATE SQLite::SQLite3)
    target_include_directories(app PRIVATE ${SQLite3_INCLUDE_DIR})
else()
    find_package(unofficial-sqlite3 CONFIG REQUIRED)
    target_link_libraries(app PRIVATE unofficial::sqlite3::sqlite3)
    target_include_directories(app PRIVATE ${unofficial-sqlite3_INCLUDE_DIRS})
endif()


find_package(CURL CONFIG REQUIRED)
target_link_libraries(app PRIVATE CURL::libcurl)
target_include_directories(app PRIVATE ${CURL_INCLUDE_DIR})
