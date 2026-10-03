#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "leaf/core/result.hpp"
#include "leaf/events/decision.hpp"
#include "leaf/events/event_arena.hpp"
#include "leaf/events/event_descriptor.hpp"
#include "leaf/events/event_header.hpp"
#include "leaf/events/event_registry.hpp"
#include "leaf/events/event_trace.hpp"
#include "leaf/events/subscription.hpp"

namespace leaf {

struct publish_options {
    event_source source{event_source::engine};
    mod_id publisher{engine_mod_id};
    std::uint32_t schema_version{1};
};

struct decision_result {
    decision final_decision{decision::pass};
    std::uint64_t sequence{0};
};

class scheduler;

class event_runtime {
public:
    static constexpr std::uint32_t max_dispatch_depth = 16;

    event_runtime();
    explicit event_runtime(event_registry registry);

    event_runtime(const event_runtime&) = delete;
    event_runtime& operator=(const event_runtime&) = delete;

    /// Optional: when set and main thread is bound, main_sync events require main.
    void attach_scheduler(scheduler* sched) noexcept { scheduler_ = sched; }

    [[nodiscard]] scheduler* attached_scheduler() const noexcept { return scheduler_; }

    [[nodiscard]] event_registry& registry() noexcept { return registry_; }
    [[nodiscard]] const event_registry& registry() const noexcept { return registry_; }

    [[nodiscard]] event_trace& trace() noexcept { return trace_; }

    [[nodiscard]] result<subscription> subscribe_notification(
        event_id id,
        notification_callback callback,
        event_priority priority = event_priority::normal,
        mod_id owner = test_mod_id);

    [[nodiscard]] result<subscription> subscribe_decision(
        event_id id,
        decision_callback callback,
        event_priority priority = event_priority::normal,
        mod_id owner = test_mod_id);

    [[nodiscard]] status unsubscribe(subscription_id id);

    /// Drop every subscription owned by `owner` (mod unload path).
    void unsubscribe_owner(mod_id owner);

    [[nodiscard]] status publish_notification(
        event_id id,
        std::span<const std::byte> payload,
        publish_options options = {},
        std::span<const std::byte> variable = {});

    [[nodiscard]] result<decision_result> publish_decision(
        event_id id,
        std::span<const std::byte> payload,
        publish_options options = {},
        std::span<const std::byte> variable = {});

    /// Helper for POD payloads.
    template <typename Payload>
    [[nodiscard]] status publish_notification(
        event_id id,
        const Payload& payload,
        publish_options options = {}) {
        const auto* bytes = reinterpret_cast<const std::byte*>(&payload);
        return publish_notification(
            id,
            std::span<const std::byte>{bytes, sizeof(Payload)},
            options);
    }

    template <typename Payload>
    [[nodiscard]] result<decision_result> publish_decision(
        event_id id,
        const Payload& payload,
        publish_options options = {}) {
        const auto* bytes = reinterpret_cast<const std::byte*>(&payload);
        return publish_decision(
            id,
            std::span<const std::byte>{bytes, sizeof(Payload)},
            options);
    }

    [[nodiscard]] std::uint64_t listener_count(event_id id) const;

    friend class subscription;

private:
    struct listener_record {
        subscription_id id{0};
        mod_id owner{0};
        event_id event{0};
        event_priority priority{event_priority::normal};
        std::uint64_t sequence{0}; // stable registration order
        notification_callback notify;
        decision_callback decide;
        bool decision_listener{false};
        bool active{true};
    };

    struct dispatch_snapshot {
        std::vector<listener_record> listeners; // sorted
        std::uint64_t generation{0};
    };

    void rebuild_snapshot_unlocked_(event_id id);
    [[nodiscard]] std::shared_ptr<const dispatch_snapshot> snapshot_for_(event_id id);

    [[nodiscard]] std::uint64_t now_ns_() const;
    [[nodiscard]] status ensure_can_dispatch_(const event_descriptor& desc);

    event_registry registry_;
    event_trace trace_;
    event_arena arena_;
    scheduler* scheduler_{nullptr};

    mutable std::mutex mutex_;
    std::unordered_map<subscription_id, listener_record> listeners_;
    std::unordered_map<event_id, std::shared_ptr<const dispatch_snapshot>> snapshots_;
    std::unordered_map<event_id, std::uint64_t> snapshot_generation_;

    std::atomic<subscription_id> next_subscription_{1};
    std::atomic<std::uint64_t> next_event_sequence_{1};
    std::atomic<std::uint64_t> next_reg_sequence_{1};

    std::uint32_t dispatch_depth_{0};
    std::uint64_t current_sequence_{0};
    std::uint64_t root_sequence_{0};
    std::uint64_t parent_sequence_{0};
};

} // namespace leaf
