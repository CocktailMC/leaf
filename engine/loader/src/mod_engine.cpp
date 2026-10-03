#include "leaf/loader/mod_engine.hpp"

#include "leaf/loader/discovery.hpp"
#include "leaf/loader/host_target.hpp"

namespace leaf {

mod_engine::mod_engine()
    : mod_engine(capability_set{}) {}

mod_engine::mod_engine(capability_set capabilities)
    : api_(std::move(capabilities))
    , host_target_(current_host_target()) {
    api_.attach_events(&events_);
    api_.attach_scheduler(&scheduler_);
    events_.attach_scheduler(&scheduler_);
}

status mod_engine::bootstrap(const engine_bootstrap_options& options) {
    if (!mods_.empty()) {
        return err(ec::mod_state_invalid, "engine already bootstrapped");
    }

    host_target_ = options.resolve.host_target.empty()
        ? current_host_target()
        : options.resolve.host_target;

    resolve_options resolve = options.resolve;
    if (resolve.host_target.empty()) {
        resolve.host_target = host_target_;
    }

    if (!options.capabilities.empty()) {
        api_.set_capabilities(options.capabilities);
    }

    auto discovered = discover_mods(options.leafmods_dir);
    if (!discovered) {
        return err(discovered.error());
    }

    auto resolved = resolve_mods(std::move(*discovered), resolve);
    if (!resolved) {
        return err(resolved.error());
    }

    for (auto& package : resolved->load_order) {
        auto instance = mod_instance::load(std::move(package), api_.abi(), host_target_);
        if (!instance) {
            // Best-effort unwind already-loaded mods.
            (void)unload_all();
            return err(instance.error());
        }
        if (auto st = instance->call_on_load(api_.abi()); !st) {
            (void)instance->unload(api_.abi());
            (void)unload_all();
            return st;
        }
        mods_.push_back(std::make_unique<mod_instance>(std::move(*instance)));
    }

    return ok();
}

status mod_engine::enable_all() {
    for (auto& mod : mods_) {
        if (mod->state() == mod_state::enabled) {
            continue;
        }
        if (auto st = mod->enable(api_.abi()); !st) {
            return st;
        }
    }
    return ok();
}

status mod_engine::disable_all() {
    // Reverse order: dependents first.
    for (auto it = mods_.rbegin(); it != mods_.rend(); ++it) {
        if ((*it)->state() != mod_state::enabled) {
            continue;
        }
        if (auto st = (*it)->disable(api_.abi()); !st) {
            return st;
        }
    }
    return ok();
}

status mod_engine::unload_all() {
    for (auto it = mods_.rbegin(); it != mods_.rend(); ++it) {
        if ((*it)->state() == mod_state::unloaded
            || (*it)->state() == mod_state::failed) {
            continue;
        }
        if (auto st = (*it)->unload(api_.abi()); !st) {
            return st;
        }
    }
    mods_.clear();
    return ok();
}

mod_instance* mod_engine::find(std::string_view id) noexcept {
    for (auto& mod : mods_) {
        if (mod->id() == id) {
            return mod.get();
        }
    }
    return nullptr;
}

} // namespace leaf
