#pragma once

#include <cstdint>
#include <string_view>

namespace leaf {

/// Lifecycle of a discovered Leaf Mod instance inside the engine.
/// Transitions are owned exclusively by LEAFMC — never by Forge/Fabric/NeoForge.
enum class mod_state : std::uint8_t {
    discovered = 0,
    resolved = 1,
    loaded = 2,
    initialized = 3,
    enabled = 4,
    disabled = 5,
    unloaded = 6,
    failed = 7,
};

[[nodiscard]] constexpr std::string_view to_string(mod_state state) noexcept {
    switch (state) {
        case mod_state::discovered: return "discovered";
        case mod_state::resolved: return "resolved";
        case mod_state::loaded: return "loaded";
        case mod_state::initialized: return "initialized";
        case mod_state::enabled: return "enabled";
        case mod_state::disabled: return "disabled";
        case mod_state::unloaded: return "unloaded";
        case mod_state::failed: return "failed";
    }
    return "unknown";
}

} // namespace leaf
