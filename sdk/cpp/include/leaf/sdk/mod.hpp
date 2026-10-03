#pragma once

#include "leaf/sdk/api.hpp"
#include "leaf/sdk/status.hpp"

#include "leaf/abi/leaf_abi_v1.h"

#include <string_view>

namespace leaf::sdk {

struct mod_info {
    std::string_view id;
    std::string_view name;
    std::string_view version;
};

/// Base class for C++23 Leaf Mods. Prefer {@link LEAF_DEFINE_MOD}.
class mod {
public:
    virtual ~mod() = default;

    [[nodiscard]] virtual mod_info info() const noexcept = 0;

    virtual void on_load(api& /*a*/) {}
    virtual void on_enable(api& /*a*/) {}
    virtual void on_disable(api& /*a*/) {}
    virtual void on_unload(api& /*a*/) {}
};

namespace detail {

template <typename ModT>
struct mod_holder {
    static ModT& instance() {
        static ModT object;
        return object;
    }

    static void on_load(const LeafApiV1* raw) {
        api wrap{raw};
        instance().on_load(wrap);
    }

    static void on_enable(const LeafApiV1* raw) {
        api wrap{raw};
        instance().on_enable(wrap);
    }

    static void on_disable(const LeafApiV1* raw) {
        api wrap{raw};
        instance().on_disable(wrap);
    }

    static void on_unload(const LeafApiV1* raw) {
        api wrap{raw};
        instance().on_unload(wrap);
    }
};

} // namespace detail

} // namespace leaf::sdk

#if defined(_WIN32) || defined(__CYGWIN__)
#  define LEAF_MOD_EXPORT __declspec(dllexport)
#else
#  define LEAF_MOD_EXPORT __attribute__((visibility("default")))
#endif

/// Export `leaf_mod_entry` for a concrete {@link leaf::sdk::mod} subclass.
/// Place once in a single translation unit of the `.leafmod` shared library.
#define LEAF_DEFINE_MOD(ModType)                                              \
    extern "C" {                                                              \
    LEAF_MOD_EXPORT LeafStatus leaf_mod_entry(                                \
        const LeafApiV1* raw_api,                                             \
        LeafModInfoV1* out_info,                                              \
        LeafModExportsV1* out_exports) {                                      \
        if (!raw_api || !out_info || !out_exports) {                          \
            return LEAF_STATUS_INVALID_ARGUMENT;                              \
        }                                                                     \
        if (raw_api->abi_major != LEAF_ABI_VERSION_MAJOR) {                   \
            return LEAF_STATUS_ABI_MISMATCH;                                  \
        }                                                                     \
        const auto meta = leaf::sdk::detail::mod_holder<ModType>::instance()  \
                              .info();                                        \
        out_info->struct_size =                                               \
            static_cast<uint32_t>(sizeof(LeafModInfoV1));                     \
        out_info->id = meta.id.data();                                        \
        out_info->name = meta.name.data();                                    \
        out_info->version = meta.version.data();                              \
        out_exports->struct_size =                                            \
            static_cast<uint32_t>(sizeof(LeafModExportsV1));                  \
        out_exports->on_load =                                                \
            &leaf::sdk::detail::mod_holder<ModType>::on_load;                 \
        out_exports->on_enable =                                              \
            &leaf::sdk::detail::mod_holder<ModType>::on_enable;               \
        out_exports->on_disable =                                             \
            &leaf::sdk::detail::mod_holder<ModType>::on_disable;              \
        out_exports->on_unload =                                              \
            &leaf::sdk::detail::mod_holder<ModType>::on_unload;               \
        return LEAF_STATUS_OK;                                                \
    }                                                                         \
    }
