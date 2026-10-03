#pragma once

#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "leaf/core/result.hpp"
#include "leaf/events/event_descriptor.hpp"

namespace leaf {

class event_registry {
public:
    event_registry();

    [[nodiscard]] status register_core(event_descriptor desc);

    [[nodiscard]] result<event_id> register_dynamic(event_descriptor desc);

    [[nodiscard]] const event_descriptor* find(event_id id) const noexcept;

    [[nodiscard]] const event_descriptor* find_by_name(std::string_view name) const noexcept;

    [[nodiscard]] const std::vector<event_descriptor>& all() const noexcept {
        return ordered_;
    }

    void unregister_owner(mod_id owner);

private:
    void index_(event_descriptor desc);

    std::vector<event_descriptor> ordered_;
    std::unordered_map<event_id, std::size_t> by_id_;
    std::unordered_map<std::string, std::size_t> by_name_;
    event_id next_dynamic_{event_ids::dynamic_id_base};
};

[[nodiscard]] event_registry make_default_event_registry();

} // namespace leaf
