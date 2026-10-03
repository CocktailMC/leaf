#include "leaf/events/events.hpp"
#include "leaf/events/payloads.hpp"
#include "test_harness.hpp"

#include <atomic>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

void test_decision_reducer() {
    using leaf::decision;
    {
        const decision votes[] = {decision::pass, decision::allow, decision::pass};
        LEAF_CHECK(leaf::reduce_decisions(votes) == decision::allow);
    }
    {
        const decision votes[] = {decision::allow, decision::deny, decision::allow};
        LEAF_CHECK(leaf::reduce_decisions(votes) == decision::deny);
    }
    {
        const decision votes[] = {decision::pass, decision::pass};
        LEAF_CHECK(leaf::reduce_decisions(votes) == decision::pass);
    }
}

void test_event_priority_order() {
    leaf::event_runtime rt;
    std::vector<std::string> order;

    auto late = rt.subscribe_notification(
        leaf::event_ids::player_join,
        [&](leaf::event_packet_view) { order.push_back("late"); },
        leaf::event_priority::late,
        10);
    LEAF_CHECK(late.has_value());

    auto early = rt.subscribe_notification(
        leaf::event_ids::player_join,
        [&](leaf::event_packet_view) { order.push_back("early"); },
        leaf::event_priority::early,
        10);
    LEAF_CHECK(early.has_value());

    auto normal_a = rt.subscribe_notification(
        leaf::event_ids::player_join,
        [&](leaf::event_packet_view) { order.push_back("normal_a"); },
        leaf::event_priority::normal,
        10);
    auto normal_b = rt.subscribe_notification(
        leaf::event_ids::player_join,
        [&](leaf::event_packet_view) { order.push_back("normal_b"); },
        leaf::event_priority::normal,
        10);
    LEAF_CHECK(normal_a.has_value());
    LEAF_CHECK(normal_b.has_value());

    leaf::player_join_payload_v1 payload{.player_handle = 42};
    LEAF_CHECK(rt.publish_notification(leaf::event_ids::player_join, payload).has_value());

    LEAF_CHECK(order.size() == 4);
    LEAF_CHECK(order[0] == "early");
    LEAF_CHECK(order[1] == "normal_a");
    LEAF_CHECK(order[2] == "normal_b");
    LEAF_CHECK(order[3] == "late");
}

void test_decision_event() {
    leaf::event_runtime rt;

    auto allow = rt.subscribe_decision(
        leaf::event_ids::player_join_request,
        [](leaf::event_packet_view) { return leaf::decision::allow; },
        leaf::event_priority::normal,
        11);
    auto deny = rt.subscribe_decision(
        leaf::event_ids::player_join_request,
        [](leaf::event_packet_view) { return leaf::decision::deny; },
        leaf::event_priority::late,
        12);
    LEAF_CHECK(allow.has_value());
    LEAF_CHECK(deny.has_value());

    leaf::player_join_request_payload_v1 payload{.player_handle = 7};
    auto result = rt.publish_decision(leaf::event_ids::player_join_request, payload);
    LEAF_CHECK(result.has_value());
    LEAF_CHECK(result->final_decision == leaf::decision::deny);
}

void test_snapshot_subscribe_during_dispatch() {
    leaf::event_runtime rt;
    std::atomic<int> first_calls{0};
    std::atomic<int> second_calls{0};
    leaf::subscription nested;

    auto first = rt.subscribe_notification(
        leaf::event_ids::player_leave,
        [&](leaf::event_packet_view) {
            ++first_calls;
            auto sub = rt.subscribe_notification(
                leaf::event_ids::player_leave,
                [&](leaf::event_packet_view) { ++second_calls; },
                leaf::event_priority::normal,
                20);
            LEAF_CHECK(sub.has_value());
            nested = std::move(*sub);
        },
        leaf::event_priority::normal,
        20);
    LEAF_CHECK(first.has_value());

    leaf::player_leave_payload_v1 payload{.player_handle = 1};
    LEAF_CHECK(rt.publish_notification(leaf::event_ids::player_leave, payload).has_value());
    LEAF_CHECK(first_calls.load() == 1);
    LEAF_CHECK(second_calls.load() == 0);

    LEAF_CHECK(rt.publish_notification(leaf::event_ids::player_leave, payload).has_value());
    LEAF_CHECK(first_calls.load() == 2);
    LEAF_CHECK(second_calls.load() == 1);
}

void test_owner_unsubscribe() {
    leaf::event_runtime rt;
    std::atomic<int> calls{0};

    auto kept = rt.subscribe_notification(
        leaf::event_ids::world_load,
        [&](leaf::event_packet_view) { ++calls; },
        leaf::event_priority::normal,
        99);
    LEAF_CHECK(kept.has_value());
    LEAF_CHECK(rt.listener_count(leaf::event_ids::world_load) == 1);

    rt.unsubscribe_owner(99);
    LEAF_CHECK(rt.listener_count(leaf::event_ids::world_load) == 0);

    struct world_load_payload_v1 {
        std::uint64_t world_handle{0};
    } world{};
    LEAF_CHECK(rt.publish_notification(leaf::event_ids::world_load, world).has_value());
    LEAF_CHECK(calls.load() == 0);
}

void test_recursive_limit() {
    leaf::event_runtime rt;
    std::atomic<bool> hit_limit{false};

    auto sub = rt.subscribe_notification(
        leaf::event_ids::entity_spawn,
        [&](leaf::event_packet_view) {
            struct payload {
                std::uint64_t entity{0};
            } p{};
            auto nested = rt.publish_notification(leaf::event_ids::entity_spawn, p);
            if (!nested
                && nested.error().code() == leaf::ec::event_recursive_limit) {
                hit_limit = true;
            }
        },
        leaf::event_priority::normal,
        3);
    LEAF_CHECK(sub.has_value());

    struct payload {
        std::uint64_t entity{0};
    } p{};
    auto st = rt.publish_notification(leaf::event_ids::entity_spawn, p);
    LEAF_CHECK(st.has_value());
    LEAF_CHECK(hit_limit.load());
}

void test_monitor_cannot_decide() {
    leaf::event_runtime rt;
    auto bad = rt.subscribe_decision(
        leaf::event_ids::player_join_request,
        [](leaf::event_packet_view) { return leaf::decision::deny; },
        leaf::event_priority::monitor,
        4);
    LEAF_CHECK(!bad.has_value());
    LEAF_CHECK(bad.error().code() == leaf::ec::event_monitor_mutation);
}

void test_event_exception_isolation() {
    leaf::event_runtime rt;
    std::atomic<int> after{0};

    auto boom = rt.subscribe_notification(
        leaf::event_ids::core_server_started,
        [](leaf::event_packet_view) { throw std::runtime_error("boom"); },
        leaf::event_priority::early,
        5);
    auto ok_sub = rt.subscribe_notification(
        leaf::event_ids::core_server_started,
        [&](leaf::event_packet_view) { ++after; },
        leaf::event_priority::normal,
        5);
    LEAF_CHECK(boom.has_value());
    LEAF_CHECK(ok_sub.has_value());

    struct empty_payload {
        std::uint32_t pad{0};
    } p{};
    auto st = rt.publish_notification(leaf::event_ids::core_server_started, p);
    LEAF_CHECK(!st.has_value());
    LEAF_CHECK(after.load() == 1);
}

void test_dynamic_event_and_trace() {
    leaf::event_runtime rt;
    rt.trace().set_enabled(true);

    leaf::event_descriptor desc;
    desc.name = "mod.economy.balance_changed";
    desc.schema_version = 1;
    desc.kind = leaf::event_kind::notification;
    desc.dispatch = leaf::dispatch_model::main_sync;
    desc.owner = 77;
    auto id = rt.registry().register_dynamic(desc);
    LEAF_CHECK(id.has_value());

    std::atomic<int> calls{0};
    auto sub = rt.subscribe_notification(
        *id,
        [&](leaf::event_packet_view view) {
            ++calls;
            LEAF_CHECK(view.id() == *id);
        },
        leaf::event_priority::normal,
        77);
    LEAF_CHECK(sub.has_value());

    struct balance_payload {
        std::uint64_t player{1};
        std::int64_t delta{100};
    } payload{};
    LEAF_CHECK(rt.publish_notification(*id, payload).has_value());
    LEAF_CHECK(calls.load() == 1);

    auto records = rt.trace().take_records();
    LEAF_CHECK(!records.empty());
    LEAF_CHECK(records.back().id == *id);
}

void test_events_bundle() {
    test_decision_reducer();
    test_event_priority_order();
    test_decision_event();
    test_snapshot_subscribe_during_dispatch();
    test_owner_unsubscribe();
    test_recursive_limit();
    test_monitor_cannot_decide();
    test_event_exception_isolation();
    test_dynamic_event_and_trace();
}
