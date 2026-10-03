add_library(leaf_events STATIC
    src/decision_reducer.cpp
    src/event_registry.cpp
    src/event_arena.cpp
    src/event_trace.cpp
    src/subscription.cpp
    src/event_runtime.cpp
)

add_library(leaf::events ALIAS leaf_events)

target_include_directories(leaf_events
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/abi/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(leaf_events
    PUBLIC
        leaf::core
        leaf::scheduler
)

target_compile_features(leaf_events PUBLIC cxx_std_${LEAF_CXX_STANDARD})
target_compile_definitions(leaf_events PRIVATE LEAF_ENGINE_BUILD=1)

leaf_apply_warnings(leaf_events)
leaf_apply_sanitizers(leaf_events)

set_target_properties(leaf_events PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME leaf_events
)
