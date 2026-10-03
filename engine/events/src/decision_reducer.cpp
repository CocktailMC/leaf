#include "leaf/events/decision.hpp"

namespace leaf {

decision reduce_decisions(
    std::span<const decision> votes,
    decision_policy policy) noexcept {
    if (policy != decision_policy::deny_over_allow) {
        // Only one policy in v1; treat unknown as deny_over_allow.
    }

    bool any_allow = false;
    for (decision v : votes) {
        if (v == decision::deny) {
            return decision::deny;
        }
        if (v == decision::allow) {
            any_allow = true;
        }
    }
    return any_allow ? decision::allow : decision::pass;
}

} // namespace leaf
