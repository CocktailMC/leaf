#pragma once

#include <cstdint>
#include <span>

#include "leaf/events/event_types.hpp"

namespace leaf {

/// Deterministic decision merge for Decision Events.
/// Policy deny_over_allow: any DENY => DENY; else any ALLOW => ALLOW; else PASS.
[[nodiscard]] decision reduce_decisions(
    std::span<const decision> votes,
    decision_policy policy = decision_policy::deny_over_allow) noexcept;

} // namespace leaf
