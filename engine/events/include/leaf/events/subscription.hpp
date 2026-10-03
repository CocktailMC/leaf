#pragma once

#include <cstdint>
#include <functional>
#include <utility>

#include "leaf/events/event_header.hpp"
#include "leaf/events/event_id.hpp"
#include "leaf/events/event_types.hpp"

namespace leaf {

class event_runtime;

/// Engine-side listener callback. Exceptions must not escape (runtime catches).
using notification_callback =
    std::function<void(event_packet_view view)>;

using decision_callback =
    std::function<decision(event_packet_view view)>;

/// RAII subscription handle. Destructor unsubscribes if still active.
/// Owner-based cleanup in event_runtime remains authoritative on mod unload.
class subscription {
public:
    subscription() = default;
    subscription(event_runtime* runtime, subscription_id id) noexcept;

    ~subscription();

    subscription(const subscription&) = delete;
    subscription& operator=(const subscription&) = delete;

    subscription(subscription&& other) noexcept;
    subscription& operator=(subscription&& other) noexcept;

    void unsubscribe() noexcept;

    [[nodiscard]] bool active() const noexcept { return runtime_ != nullptr && id_ != 0; }

    [[nodiscard]] subscription_id id() const noexcept { return id_; }

private:
    event_runtime* runtime_{nullptr};
    subscription_id id_{0};
};

} // namespace leaf
