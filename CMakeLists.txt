cmake_minimum_required(VERSION 4.0.0)
project(xps_controller VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(GTest CONFIG REQUIRED)

add_library(newport_xps SHARED
    src/xps_controller.cc
    src/xps_group.cc
    src/xps_positioner.cc
    src/internal/tcp_socket.cc
)

target_include_directories(newport_xps
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(newport_xps PRIVATE NEWPORT_XPS_BUILDING_DLL)

# Winsock2 is required for TCP socket support on Windows.
if(WIN32)
    target_link_libraries(newport_xps PRIVATE Ws2_32)
endif()

target_compile_options(newport_xps PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

enable_testing()
add_subdirectory(tests)
