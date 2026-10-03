# Example C++23 Leaf Mod using the Leaf SDK (header-only).

add_library(leaf_example_greeter MODULE
    src/greeter_mod.cpp
)

target_link_libraries(leaf_example_greeter PRIVATE leaf::sdk)

target_compile_features(leaf_example_greeter PRIVATE cxx_std_${LEAF_CXX_STANDARD})
set_target_properties(leaf_example_greeter PROPERTIES
    PREFIX "lib"
    OUTPUT_NAME "greeter"
    POSITION_INDEPENDENT_CODE ON
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

leaf_apply_warnings(leaf_example_greeter)

# Host triple for packaging (mirrors leaf::current_host_target).
if(WIN32)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64")
        set(LEAF_GREETER_HOST_TARGET "windows-arm64")
    else()
        set(LEAF_GREETER_HOST_TARGET "windows-x86_64")
    endif()
    set(LEAF_GREETER_LIB_NAME "greeter.dll")
elseif(APPLE)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(LEAF_GREETER_HOST_TARGET "macos-arm64")
    else()
        set(LEAF_GREETER_HOST_TARGET "macos-x86_64")
    endif()
    set(LEAF_GREETER_LIB_NAME "libgreeter.dylib")
else()
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(LEAF_GREETER_HOST_TARGET "linux-aarch64")
    else()
        set(LEAF_GREETER_HOST_TARGET "linux-x86_64")
    endif()
    set(LEAF_GREETER_LIB_NAME "libgreeter.so")
endif()

set(LEAF_GREETER_LEAFMOD_DIR
    "${CMAKE_BINARY_DIR}/tests/greeter_leafmods/greeter.leafmod")

add_custom_command(
    TARGET leaf_example_greeter
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${LEAF_GREETER_LEAFMOD_DIR}/native/${LEAF_GREETER_HOST_TARGET}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/leaf.mod.json"
        "${LEAF_GREETER_LEAFMOD_DIR}/leaf.mod.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:leaf_example_greeter>"
        "${LEAF_GREETER_LEAFMOD_DIR}/native/${LEAF_GREETER_HOST_TARGET}/${LEAF_GREETER_LIB_NAME}"
    COMMENT "Packaging greeter.leafmod for bridge tests"
    VERBATIM
)
