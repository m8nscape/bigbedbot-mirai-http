cmake_minimum_required(VERSION 3.8...3.18)

add_library(app STATIC 
    common/dbconn.cpp common/dbconn.h
    data/group.cpp data/group.h data/user.cpp data/user.h
    api/api.cpp api/api.h api/api_user.cpp api/api_group.cpp api/api_command.cpp
    case.cpp case.h
    help.cpp help.h
    tools.cpp tools.h
    smoke.cpp smoke.h
    eatwhat.cpp eatwhat.h
    monopoly.cpp monopoly.h
    weather.cpp weather.h
    gambol.cpp gambol.h
    apievent.cpp apievent.h
    playwhat.cpp playwhat.h
)
set_target_properties(app PROPERTIES
    CXX_STANDARD 17)
target_link_libraries(app 
    PRIVATE mirai-api
    PRIVATE json
    PRIVATE yaml-cpp
)
target_include_directories(app 
    PRIVATE ${CMAKE_SOURCE_DIR}
)
target_precompile_headers(app
    PRIVATE pch.h
)

find_package(SQLite3 CONFIG)
if (SQLite3_FOUND)
    set(sqlite3_LIBRARY SQLite::SQLite3)
    target_link_libraries(app PRIVATE ${sqlite3_LIBRARY})
else()
    find_package(unofficial-sqlite3 CONFIG)
    if (unofficial-sqlite3_FOUND)
        set(sqlite3_LIBRARY unofficial::sqlite3::sqlite3)
        target_link_libraries(app PRIVATE ${sqlite3_LIBRARY})
    else()
        message("Using libsqlite3 from system")
        set(sqlite3_LIBRARY -lsqlite3)
        target_link_libraries(app PRIVATE ${sqlite3_LIBRARY})
    endif()
endif()

target_link_libraries(app PRIVATE ${CURL_LIBRARY})

target_link_libraries(app PRIVATE Boost::disable_autolinking)