# nlohmann/json
#set(JSON_BuildTests OFF CACHE INTERNAL "")
#add_subdirectory(nlohmann_json)
# the repository is kinda huge so I just kept the single header version
add_library(json INTERFACE)
target_include_directories(json 
    INTERFACE nlohmann/single-include)

# jbeder/yaml-cpp
option(YAML_CPP_BUILD_TOOLS "Enable parse tools" OFF)
add_subdirectory(yaml-cpp)