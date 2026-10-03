#include "leaf/loader/mod_instance.hpp"

#include "leaf/loader/native_path.hpp"

#include <exception>

namespace leaf {
namespace {

template <typename Fn>
[[nodiscard]] status invoke_mod_hook(Fn&& fn, std::string_view what) {
    try {
        std::forward<Fn>(fn)();
        return ok();
    } catch (const std::exception& ex) {
        return err(
            ec::mod_load_failed,
            std::string(what) + " threw: " + ex.what());
    } catch (...) {
        return err(
            ec::mod_load_failed,
            std::string(what) + " threw unknown exception");
    }
}

} // namespace

mod_instance::mod_instance(discovered_mod package, shared_library library)
    : package_(std::move(package))
    , library_(std::move(library)) {}

result<mod_instance> mod_instance::load(
    discovered_mod package,
    const LeafApiV1& api,
    std::string_view host_target) {
    auto lib_path = resolve_native_library_path(package, host_target);
    if (!lib_path) {
        package.state = mod_state::failed;
        return err<mod_instance>(lib_path.error());
    }

    auto library = shared_library::open(*lib_path);
    if (!library) {
        package.state = mod_state::failed;
        return err<mod_instance>(library.error());
    }

    auto sym = library->symbol(package.manifest.entry);
    if (!sym) {
        package.state = mod_state::failed;
        return err<mod_instance>(sym.error());
    }

    auto* entry = reinterpret_cast<LeafModEntryFn>(*sym);

    LeafModInfoV1 info{};
    info.struct_size = static_cast<uint32_t>(sizeof(LeafModInfoV1));

    LeafModExportsV1 exports{};
    exports.struct_size = static_cast<uint32_t>(sizeof(LeafModExportsV1));

    LeafStatus st = LEAF_STATUS_ERROR;
    try {
        st = entry(&api, &info, &exports);
    } catch (const std::exception& ex) {
        package.state = mod_state::failed;
        return err<mod_instance>(
            ec::mod_load_failed,
            "leaf_mod_entry threw: " + std::string(ex.what()));
    } catch (...) {
        package.state = mod_state::failed;
        return err<mod_instance>(
            ec::mod_load_failed,
            "leaf_mod_entry threw unknown exception");
    }

    if (st != LEAF_STATUS_OK) {
        package.state = mod_state::failed;
        return err<mod_instance>(
            ec::mod_load_failed,
            "leaf_mod_entry returned status " + std::to_string(static_cast<int>(st)));
    }

    if (exports.struct_size < sizeof(LeafModExportsV1)) {
        package.state = mod_state::failed;
        return err<mod_instance>(
            ec::abi_version_mismatch,
            "LeafModExportsV1 struct_size too small");
    }

    // Optional consistency check against manifest id.
    if (info.id != nullptr && package.manifest.id != info.id) {
        package.state = mod_state::failed;
        return err<mod_instance>(
            ec::mod_manifest_invalid,
            "entry id '" + std::string(info.id) + "' != manifest id '"
                + package.manifest.id + "'");
    }

    mod_instance instance{std::move(package), std::move(*library)};
    instance.exports_ = exports;
    instance.exports_valid_ = true;
    instance.package_.state = mod_state::loaded;
    return instance;
}

status mod_instance::call_on_load(const LeafApiV1& api) {
    if (package_.state != mod_state::loaded) {
        return err(ec::mod_state_invalid, "expected LOADED");
    }
    if (!exports_valid_ || !exports_.on_load) {
        package_.state = mod_state::initialized;
        return ok();
    }
    auto st = invoke_mod_hook([&] { exports_.on_load(&api); }, "on_load");
    if (!st) {
        package_.state = mod_state::failed;
        return st;
    }
    package_.state = mod_state::initialized;
    return ok();
}

status mod_instance::enable(const LeafApiV1& api) {
    if (package_.state != mod_state::initialized
        && package_.state != mod_state::disabled) {
        return err(ec::mod_state_invalid, "expected INITIALIZED or DISABLED");
    }
    if (exports_valid_ && exports_.on_enable) {
        auto st = invoke_mod_hook([&] { exports_.on_enable(&api); }, "on_enable");
        if (!st) {
            package_.state = mod_state::failed;
            return st;
        }
    }
    package_.state = mod_state::enabled;
    return ok();
}

status mod_instance::disable(const LeafApiV1& api) {
    if (package_.state != mod_state::enabled) {
        return err(ec::mod_state_invalid, "expected ENABLED");
    }
    if (exports_valid_ && exports_.on_disable) {
        auto st = invoke_mod_hook([&] { exports_.on_disable(&api); }, "on_disable");
        if (!st) {
            package_.state = mod_state::failed;
            return st;
        }
    }
    package_.state = mod_state::disabled;
    return ok();
}

status mod_instance::unload(const LeafApiV1& api) {
    if (package_.state == mod_state::enabled) {
        if (auto st = disable(api); !st) {
            return st;
        }
    }
    if (package_.state != mod_state::disabled
        && package_.state != mod_state::initialized
        && package_.state != mod_state::loaded) {
        return err(ec::mod_state_invalid, "cannot unload from current state");
    }
    if (exports_valid_ && exports_.on_unload) {
        auto st = invoke_mod_hook([&] { exports_.on_unload(&api); }, "on_unload");
        if (!st) {
            package_.state = mod_state::failed;
            return st;
        }
    }
    exports_ = {};
    exports_valid_ = false;
    library_.close();
    package_.state = mod_state::unloaded;
    return ok();
}

} // namespace leaf
