# --- unit test executable ---------------------------------------------------

add_executable(leaf_tests
    test_main.cpp
    test_version.cpp
    test_capability.cpp
    test_handle_table.cpp
    test_abi_header.cpp
    test_version_range.cpp
    test_manifest_parser.cpp
    test_discovery_resolve.cpp
    test_native_lifecycle.cpp
    test_event_runtime.cpp
    test_error_codes.cpp
    test_scheduler.cpp
    test_bridge_host.cpp
    test_bridge_greeter.cpp
    test_zip_leafmod.cpp
    test_minecraft_abi.cpp
    test_sdk.cpp
)

target_link_libraries(leaf_tests
    PRIVATE
        leaf::core
        leaf::object
        leaf::scheduler
        leaf::events
        leaf::minecraft_abi
        leaf::loader
        leaf::bridge
        leaf::sdk
)

target_compile_definitions(leaf_tests
    PRIVATE
        LEAF_TEST_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
        LEAF_TEST_RUNTIME_LEAFMODS_DIR="${CMAKE_CURRENT_BINARY_DIR}/runtime_leafmods"
        LEAF_TEST_GREETER_LEAFMODS_DIR="${CMAKE_CURRENT_BINARY_DIR}/greeter_leafmods"
)

target_compile_features(leaf_tests PRIVATE cxx_std_${LEAF_CXX_STANDARD})
leaf_apply_warnings(leaf_tests)
leaf_apply_sanitizers(leaf_tests)

if(UNIX AND NOT APPLE)
    target_link_libraries(leaf_tests PRIVATE dl)
endif()

# --- sample native Leaf Mod used by lifecycle tests -------------------------

add_library(leaf_hello_mod MODULE
    native_mods/hello_mod.cpp
)

target_include_directories(leaf_hello_mod
    PRIVATE
        ${CMAKE_SOURCE_DIR}/abi/include
)

target_compile_features(leaf_hello_mod PRIVATE cxx_std_${LEAF_CXX_STANDARD})
set_target_properties(leaf_hello_mod PROPERTIES
    PREFIX "lib"
    OUTPUT_NAME "hello"
    POSITION_INDEPENDENT_CODE ON
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/hello_mod_build"
)

leaf_apply_warnings(leaf_hello_mod)

# Detect host target triple for packaging (mirrors leaf::current_host_target).
if(WIN32)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64")
        set(LEAF_HOST_TARGET "windows-arm64")
    else()
        set(LEAF_HOST_TARGET "windows-x86_64")
    endif()
    set(LEAF_HELLO_LIB_NAME "hello.dll")
elseif(APPLE)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(LEAF_HOST_TARGET "macos-arm64")
    else()
        set(LEAF_HOST_TARGET "macos-x86_64")
    endif()
    set(LEAF_HELLO_LIB_NAME "libhello.dylib")
else()
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(LEAF_HOST_TARGET "linux-aarch64")
    else()
        set(LEAF_HOST_TARGET "linux-x86_64")
    endif()
    set(LEAF_HELLO_LIB_NAME "libhello.so")
endif()

set(LEAF_RUNTIME_HELLO_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/runtime_leafmods/hello.leafmod")

add_custom_command(
    TARGET leaf_hello_mod
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${LEAF_RUNTIME_HELLO_DIR}/native/${LEAF_HOST_TARGET}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/hello_leafmod_template/leaf.mod.json"
        "${LEAF_RUNTIME_HELLO_DIR}/leaf.mod.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:leaf_hello_mod>"
        "${LEAF_RUNTIME_HELLO_DIR}/native/${LEAF_HOST_TARGET}/${LEAF_HELLO_LIB_NAME}"
    COMMENT "Packaging hello.leafmod for runtime tests"
    VERBATIM
)

add_dependencies(leaf_tests leaf_hello_mod)

# greeter.leafmod packaging lives on leaf_example_greeter (examples/cpp23-mod).
if(TARGET leaf_example_greeter)
    add_dependencies(leaf_tests leaf_example_greeter)
endif()

add_test(NAME leaf_tests COMMAND leaf_tests)
