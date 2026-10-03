add_library(leaf_loader STATIC
    src/version_range.cpp
    src/manifest_parser.cpp
    src/discovery.cpp
    src/leafmod_package.cpp
    src/dependency_graph.cpp
    src/host_target.cpp
    src/shared_library.cpp
    src/native_path.cpp
    src/runtime_api.cpp
    src/mod_instance.cpp
    src/mod_engine.cpp
)

add_library(leaf::loader ALIAS leaf_loader)

target_include_directories(leaf_loader
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/abi/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(leaf_loader
    PUBLIC
        leaf::core
        leaf::events
        leaf::minecraft_abi
)

if(UNIX AND NOT APPLE)
    target_link_libraries(leaf_loader PUBLIC dl)
endif()

target_compile_features(leaf_loader PUBLIC cxx_std_${LEAF_CXX_STANDARD})
target_compile_definitions(leaf_loader PRIVATE LEAF_ENGINE_BUILD=1)

leaf_apply_warnings(leaf_loader)
leaf_apply_sanitizers(leaf_loader)

set_target_properties(leaf_loader PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME leaf_loader
)
