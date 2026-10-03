#pragma once

#include <string>

#include "leaf/abi/leaf_abi_v1.h"
#include "leaf/core/result.hpp"
#include "leaf/loader/manifest.hpp"
#include "leaf/loader/shared_library.hpp"

namespace leaf {

/// One loaded Leaf Mod: owns the shared library and drives lifecycle hooks.
class mod_instance {
public:
    mod_instance(discovered_mod package, shared_library library);

    mod_instance(const mod_instance&) = delete;
    mod_instance& operator=(const mod_instance&) = delete;

    mod_instance(mod_instance&&) noexcept = default;
    mod_instance& operator=(mod_instance&&) noexcept = default;

    [[nodiscard]] static result<mod_instance> load(
        discovered_mod package,
        const LeafApiV1& api,
        std::string_view host_target);

    [[nodiscard]] status call_on_load(const LeafApiV1& api);
    [[nodiscard]] status enable(const LeafApiV1& api);
    [[nodiscard]] status disable(const LeafApiV1& api);
    [[nodiscard]] status unload(const LeafApiV1& api);

    [[nodiscard]] const discovered_mod& package() const noexcept {
        return package_;
    }

    [[nodiscard]] mod_state state() const noexcept { return package_.state; }

    [[nodiscard]] const std::string& id() const noexcept {
        return package_.manifest.id;
    }

private:
    discovered_mod package_;
    shared_library library_;
    LeafModExportsV1 exports_{};
    bool exports_valid_{false};
};

} // namespace leaf
