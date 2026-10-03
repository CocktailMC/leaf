#pragma once

#include <cstdint>
#include <string_view>

namespace leaf {

enum class event_kind : std::uint8_t {
    notification = 1,
    decision = 2,
    internal_signal = 3,
};

enum class dispatch_model : std::uint8_t {
    main_sync = 1,
    worker_async = 2,
    parallel_readonly = 3,
};

enum class event_priority : std::uint8_t {
    earliest = 0,
    early = 1,
    normal = 2,
    late = 3,
    latest = 4,
    monitor = 5,
};

enum class decision : std::uint8_t {
    pass = 0,
    allow = 1,
    deny = 2,
};

enum class decision_policy : std::uint8_t {
    /// DENY wins over ALLOW wins over PASS. Deterministic.
    deny_over_allow = 1,
};

enum class event_source : std::uint32_t {
    unknown = 0,
    engine = 1,
    bridge_fabric = 2,
    bridge_forge = 3,
    bridge_neoforge = 4,
    test = 5,
    mod = 6,
};

[[nodiscard]] constexpr std::string_view to_string(event_priority p) noexcept {
    switch (p) {
        case event_priority::earliest: return "earliest";
        case event_priority::early: return "early";
        case event_priority::normal: return "normal";
        case event_priority::late: return "late";
        case event_priority::latest: return "latest";
        case event_priority::monitor: return "monitor";
    }
    return "unknown";
}

[[nodiscard]] constexpr std::string_view to_string(decision d) noexcept {
    switch (d) {
        case decision::pass: return "pass";
        case decision::allow: return "allow";
        case decision::deny: return "deny";
    }
    return "unknown";
}

[[nodiscard]] constexpr std::string_view to_string(event_kind k) noexcept {
    switch (k) {
        case event_kind::notification: return "notification";
        case event_kind::decision: return "decision";
        case event_kind::internal_signal: return "internal_signal";
    }
    return "unknown";
}

} // namespace leaf
