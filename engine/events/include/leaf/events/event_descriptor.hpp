#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "leaf/events/event_id.hpp"
#include "leaf/events/event_types.hpp"

namespace leaf {

struct event_descriptor {
    event_id id{0};
    std::string name; // e.g. "leaf.player.join"
    std::uint32_t schema_version{1};
    event_kind kind{event_kind::notification};
    dispatch_model dispatch{dispatch_model::main_sync};
    decision_policy policy{decision_policy::deny_over_allow};
    std::uint32_t flags{0};
    bool core{true};
    mod_id owner{engine_mod_id};
};

} // namespace leaf
