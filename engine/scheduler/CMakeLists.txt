add_library(leaf_scheduler STATIC
    src/scheduler.cpp
)

add_library(leaf::scheduler ALIAS leaf_scheduler)

target_include_directories(leaf_scheduler
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/abi/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(leaf_scheduler
    PUBLIC
        leaf::core
)

find_package(Threads REQUIRED)
target_link_libraries(leaf_scheduler PUBLIC Threads::Threads)

target_compile_features(leaf_scheduler PUBLIC cxx_std_${LEAF_CXX_STANDARD})
target_compile_definitions(leaf_scheduler PRIVATE LEAF_ENGINE_BUILD=1)

leaf_apply_warnings(leaf_scheduler)
leaf_apply_sanitizers(leaf_scheduler)

set_target_properties(leaf_scheduler PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME leaf_scheduler
)
