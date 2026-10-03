add_library(leaf_object STATIC
    src/handle_table.cpp
)

add_library(leaf::object ALIAS leaf_object)

target_include_directories(leaf_object
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(leaf_object
    PUBLIC
        leaf::core
)

target_compile_features(leaf_object PUBLIC cxx_std_${LEAF_CXX_STANDARD})
target_compile_definitions(leaf_object PRIVATE LEAF_ENGINE_BUILD=1)

leaf_apply_warnings(leaf_object)
leaf_apply_sanitizers(leaf_object)

set_target_properties(leaf_object PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME leaf_object
)
