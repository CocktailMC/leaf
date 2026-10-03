add_library(leaf_bridge SHARED
    src/bridge_host.cpp
)

add_library(leaf::bridge ALIAS leaf_bridge)

target_include_directories(leaf_bridge
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/abi/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(leaf_bridge
    PUBLIC
        leaf::loader
        leaf::events
        leaf::scheduler
        leaf::core
)

target_compile_features(leaf_bridge PUBLIC cxx_std_${LEAF_CXX_STANDARD})
target_compile_definitions(leaf_bridge PRIVATE LEAF_BRIDGE_BUILD=1 LEAF_ENGINE_BUILD=1)

leaf_apply_warnings(leaf_bridge)
leaf_apply_sanitizers(leaf_bridge)

set_target_properties(leaf_bridge PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME leaf_bridge
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)
