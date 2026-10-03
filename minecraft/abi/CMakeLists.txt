add_library(leaf_minecraft_abi STATIC
    src/stub_minecraft_abi.cpp
)

add_library(leaf::minecraft_abi ALIAS leaf_minecraft_abi)

target_include_directories(leaf_minecraft_abi
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(leaf_minecraft_abi
    PUBLIC
        leaf::core
        leaf::object
)

target_compile_features(leaf_minecraft_abi PUBLIC cxx_std_${LEAF_CXX_STANDARD})
target_compile_definitions(leaf_minecraft_abi PRIVATE LEAF_ENGINE_BUILD=1)

leaf_apply_warnings(leaf_minecraft_abi)
leaf_apply_sanitizers(leaf_minecraft_abi)

set_target_properties(leaf_minecraft_abi PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME leaf_minecraft_abi
)
