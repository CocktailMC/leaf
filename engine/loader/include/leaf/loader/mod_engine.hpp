#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "leaf/core/result.hpp"
#include "leaf/events/event_runtime.hpp"
#include "leaf/loader/dependency_graph.hpp"
#include "leaf/loader/mod_instance.hpp"
#include "leaf/loader/runtime_api.hpp"
#include "leaf/scheduler/scheduler.hpp"

namespace leaf {

struct engine_bootstrap_options {
    std::filesystem::path leafmods_dir;
    resolve_options resolve{};
    capability_set capabilities{};
};

/// Orchestrates discover → resolve → load → initialize → enable.
class mod_engine {
public:
    mod_engine();
    explicit mod_engine(capability_set capabilities);

    [[nodiscard]] runtime_api& api() noexcept { return api_; }
    [[nodiscard]] const runtime_api& api() const noexcept { return api_; }

    [[nodiscard]] event_runtime& events() noexcept { return events_; }
    [[nodiscard]] const event_runtime& events() const noexcept { return events_; }

    [[nodiscard]] scheduler& schedule() noexcept { return scheduler_; }
    [[nodiscard]] const scheduler& schedule() const noexcept { return scheduler_; }

    [[nodiscard]] status bootstrap(const engine_bootstrap_options& options);

    [[nodiscard]] status enable_all();
    [[nodiscard]] status disable_all();
    [[nodiscard]] status unload_all();

    [[nodiscard]] const std::vector<std::unique_ptr<mod_instance>>& mods()
        const noexcept {
        return mods_;
    }

    [[nodiscard]] mod_instance* find(std::string_view id) noexcept;

private:
    scheduler scheduler_;
    event_runtime events_;
    runtime_api api_;
    std::vector<std::unique_ptr<mod_instance>> mods_;
    std::string host_target_;
};

} // namespace leaf
