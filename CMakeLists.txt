cmake_minimum_required(VERSION 3.28)

project(LEAFMC
    VERSION 0.1.0
    DESCRIPTION "Native cross-loader, cross-version Minecraft mod runtime"
    LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(LeafOptions)

set(CMAKE_CXX_STANDARD ${LEAF_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

add_subdirectory(engine/core)
add_subdirectory(engine/object-table)
add_subdirectory(engine/scheduler)
add_subdirectory(engine/events)
add_subdirectory(minecraft/abi)
add_subdirectory(engine/loader)
add_subdirectory(engine/bridge-host)
add_subdirectory(sdk/cpp)

if(LEAF_BUILD_EXAMPLES)
    add_subdirectory(examples/cpp23-mod)
endif()

if(LEAF_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

message(STATUS "LEAFMC ${PROJECT_VERSION}")
message(STATUS "  C++ standard : ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build tests  : ${LEAF_BUILD_TESTS}")
