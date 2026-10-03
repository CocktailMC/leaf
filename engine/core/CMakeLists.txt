add_library(leaf_core STATIC
    src/core.cpp
    src/error_registry.cpp
)

add_library(leaf::core ALIAS leaf_core)

target_include_directories(leaf_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/abi/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(leaf_core PUBLIC cxx_std_${LEAF_CXX_STANDARD})
target_compile_definitions(leaf_core PRIVATE LEAF_ENGINE_BUILD=1)

leaf_apply_warnings(leaf_core)
leaf_apply_sanitizers(leaf_core)

set_target_properties(leaf_core PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME leaf_core
)
