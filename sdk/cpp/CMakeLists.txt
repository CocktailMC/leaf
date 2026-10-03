# Leaf C++23 SDK (header-only)

add_library(leaf_sdk INTERFACE)
add_library(leaf::sdk ALIAS leaf_sdk)

target_include_directories(leaf_sdk
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/abi/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(leaf_sdk INTERFACE cxx_std_${LEAF_CXX_STANDARD})
