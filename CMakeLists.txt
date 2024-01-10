cmake_minimum_required(VERSION 3.21)
project(BaseKit)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Base

file(GLOB BASE_HEADERS "base/*.h")
file(GLOB BASE_SRC "base/impl/*.c")

add_library(base STATIC ${BASE_HEADERS} ${BASE_SRC})
set_target_properties(base PROPERTIES LINKER_LANGUAGE C)

# OS

file(GLOB OS_HEADERS "os/*.h")
file(GLOB OS_SRC "os/impl/*.c")

add_library(os STATIC ${OS_HEADERS} ${OS_SRC})
target_link_libraries(os base)
set_target_properties(os PROPERTIES LINKER_LANGUAGE C)

# LOG

file(GLOB LOG_HEADERS "log/*.h")
file(GLOB LOG_SRC "log/impl/*.c")

add_library(log STATIC ${LOG_HEADERS} ${LOG_SRC})
target_link_libraries(log base os)
set_target_properties(log PROPERTIES LINKER_LANGUAGE C)

# APP

file(GLOB APP_HEADERS "app/*.h")
if(APPLE)
    file(GLOB APP_SRC "app/impl/*.m")
else()
    file(GLOB APP_SRC "app/impl/*.c")
endif()

add_library(app STATIC ${APP_HEADERS} ${APP_SRC})
target_link_libraries(app os base)
set_target_properties(app PROPERTIES LINKER_LANGUAGE C)

if(APPLE)
    target_compile_options(app PUBLIC "-fobjc-arc -ObjC -dead_strip -lpthread")
    target_link_libraries(
        app
        "-framework QuartzCore"
        "-framework Cocoa"
        "-framework MetalKit"
        "-framework Metal"
    )
endif()

# tests

add_subdirectory(_tests)
#include_directories(_tests)
#if(PROJECT_IS_TOP_LEVEL)
#endif()