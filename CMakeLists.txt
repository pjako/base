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
target_link_libraries(app base os log)

if(APPLE)
    target_compile_options(app PUBLIC "-objC -fobjc-arc -lpthread") # -dead_strip
    target_link_libraries(
        app
        "-framework QuartzCore"
        "-framework Cocoa"
        "-framework MetalKit"
        "-framework Metal"
     #   "-framework OpenGLES"
        "-framework OpenGL"
    )
else()
set_target_properties(app PROPERTIES LINKER_LANGUAGE C)
endif()

# Parser

file(GLOB PARSER_HEADERS "parser/*.h")
file(GLOB PARSER_SRC "parser/impl/*.c")

add_library(parser STATIC ${PARSER_HEADERS} ${PARSER_SRC})
set_target_properties(parser PROPERTIES LINKER_LANGUAGE C)

# RX

file(GLOB RX_HEADERS "rx/*.h")
if(APPLE)
    file(GLOB RX_SRC "rx/impl/*.m")
else()
    file(GLOB RX_SRC "rx/impl/*.c")
endif()

add_library(rx STATIC ${RX_HEADERS} ${RX_SRC})
target_link_libraries(rx os base)

if(APPLE)
    target_compile_options(app PUBLIC "-objC -fobjc-arc -lpthread") # -dead_strip
    target_link_libraries(
        rx
        "-framework QuartzCore"
        "-framework Cocoa"
        "-framework MetalKit"
        "-framework Metal"
       # "-framework OpenGLES"
        "-framework OpenGL"
    )
    else()
    set_target_properties(app PROPERTIES LINKER_LANGUAGE C)
    endif()

# SHD

add_library(spirvCross STATIC
    shd/impl/deps/spirv_cross/spirv_cfg.cpp
    shd/impl/deps/spirv_cross/spirv_cpp.cpp
    shd/impl/deps/spirv_cross/spirv_cross_c.cpp
    shd/impl/deps/spirv_cross/spirv_cross_parsed_ir.cpp
    shd/impl/deps/spirv_cross/spirv_cross_util.cpp
    shd/impl/deps/spirv_cross/spirv_cross.cpp
    shd/impl/deps/spirv_cross/spirv_glsl.cpp
    shd/impl/deps/spirv_cross/spirv_hlsl.cpp
    shd/impl/deps/spirv_cross/spirv_msl.cpp
    shd/impl/deps/spirv_cross/spirv_parser.cpp
    shd/impl/deps/spirv_cross/spirv_reflect.cpp
)

set_property(TARGET spirvCross PROPERTY CXX_STANDARD 20)

add_executable(shdc shd/impl/dxc_wrapper.cc shd/impl/shd_main.cc)
set_property(TARGET shdc PROPERTY CXX_STANDARD 20)
target_link_libraries(shdc base os parser log spirv_cross)

function(app app_name)
    if(APPLE)
        add_executable(${app_name} MACOSX_BUNDLE ${ARGN})
    elseif(WIN32)
        add_executable(${app_name} WIN32 ${ARGN})
    elseif(ANDROID)
        add_executable(${app_name} SHARED ${ARGN})
    else()
        add_executable(${app_name} ${ARGN})
    endif()
endfunction()


# tests
add_subdirectory(_tests)


# examples
add_subdirectory(_examples)


#include_directories(_tests)
#if(PROJECT_IS_TOP_LEVEL)
#endif()