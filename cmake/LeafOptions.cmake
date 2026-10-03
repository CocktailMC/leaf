# LEAFMC build options and common compile settings.

include_guard(GLOBAL)

option(LEAF_BUILD_TESTS "Build LEAFMC unit tests" ON)
option(LEAF_BUILD_EXAMPLES "Build LEAFMC example mods" ON)
option(LEAF_ENABLE_ASAN "Enable AddressSanitizer (Debug only)" OFF)
option(LEAF_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer (Debug only)" OFF)

set(LEAF_CXX_STANDARD 23 CACHE STRING "C++ standard for LEAFMC")
set_property(CACHE LEAF_CXX_STANDARD PROPERTY STRINGS 23)

function(leaf_apply_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wshadow
            -Wnon-virtual-dtor
            -Wold-style-cast
        )
    endif()
endfunction()

function(leaf_apply_sanitizers target)
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        return()
    endif()
    if(LEAF_ENABLE_ASAN)
        target_compile_options(${target} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address)
    endif()
    if(LEAF_ENABLE_UBSAN)
        target_compile_options(${target} PRIVATE -fsanitize=undefined)
        target_link_options(${target} PRIVATE -fsanitize=undefined)
    endif()
endfunction()
