#include "leaf/abi/leaf_abi_v1.h"

#include <atomic>
#include <cstring>

namespace {

std::atomic<int> g_on_load{0};
std::atomic<int> g_on_enable{0};
std::atomic<int> g_on_disable{0};
std::atomic<int> g_on_unload{0};
const LeafApiV1* g_api{nullptr};

void on_load(const LeafApiV1* api) {
    g_api = api;
    ++g_on_load;
    if (api && api->log) {
        api->log(LEAF_LOG_INFO, "hello mod on_load");
    }
}

void on_enable(const LeafApiV1* api) {
    ++g_on_enable;
    if (api && api->log) {
        api->log(LEAF_LOG_INFO, "hello mod on_enable");
    }
}

void on_disable(const LeafApiV1* api) {
    ++g_on_disable;
    (void)api;
}

void on_unload(const LeafApiV1* api) {
    ++g_on_unload;
    g_api = nullptr;
    (void)api;
}

} // namespace

extern "C" {

#if defined(_WIN32)
#  define HELLO_EXPORT __declspec(dllexport)
#else
#  define HELLO_EXPORT __attribute__((visibility("default")))
#endif

HELLO_EXPORT int leaf_test_hello_on_load_count(void) {
    return g_on_load.load();
}

HELLO_EXPORT int leaf_test_hello_on_enable_count(void) {
    return g_on_enable.load();
}

HELLO_EXPORT int leaf_test_hello_on_disable_count(void) {
    return g_on_disable.load();
}

HELLO_EXPORT int leaf_test_hello_on_unload_count(void) {
    return g_on_unload.load();
}

HELLO_EXPORT LeafStatus leaf_mod_entry(
    const LeafApiV1* api,
    LeafModInfoV1* out_info,
    LeafModExportsV1* out_exports) {
    if (!api || !out_info || !out_exports) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    if (api->abi_major != LEAF_ABI_VERSION_MAJOR) {
        return LEAF_STATUS_ABI_MISMATCH;
    }

    out_info->struct_size = static_cast<uint32_t>(sizeof(LeafModInfoV1));
    out_info->id = "hello";
    out_info->name = "Hello Mod";
    out_info->version = "1.0.0";

    out_exports->struct_size = static_cast<uint32_t>(sizeof(LeafModExportsV1));
    out_exports->on_load = &on_load;
    out_exports->on_enable = &on_enable;
    out_exports->on_disable = &on_disable;
    out_exports->on_unload = &on_unload;

    return LEAF_STATUS_OK;
}

} // extern "C"
