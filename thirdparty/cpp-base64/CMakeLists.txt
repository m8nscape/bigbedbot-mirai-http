add_library(base64 STATIC 
    base64.cpp base64.h)
target_link_libraries(bbb PUBLIC base64)
