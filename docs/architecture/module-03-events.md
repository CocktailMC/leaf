# Module 3 — Event System / Event Bus v1

## 1. Responsibilities

`engine/events` implements the canonical Leaf Event Runtime.

Loader events (Fabric/Forge/NeoForge) are **inputs only**. They must be
translated into Leaf Event Packets before any Leaf Mod sees them.

## 2. Design locked in v1

| Principle | Implementation |
|-----------|----------------|
| Stable Event ID | `uint64_t` registry (`leaf.player.join` = `0x0100`) |
| No RTTI IDs | no `typeid` / `type_index` |
| Schema version | per-event `schema_version` in header |
| ABI-safe payload | POD + optional variable bytes; contiguous packet |
| Notification vs Decision | separate subscribe/publish APIs |
| Decision merge | `deny > allow > pass` (`deny_over_allow`) |
| Fixed priorities | earliest…monitor (monitor cannot decide) |
| Stable order | priority → registration sequence |
| Subscription handle | RAII `subscription` + owner cleanup |
| Snapshot dispatch | COW listener snapshot per event id |
| Recursion guard | `max_dispatch_depth = 16` |
| Trace/metrics | optional `event_trace` |
| Arena | bump allocator, reset after dispatch |

## 3. Layout

```text
abi/include/leaf/abi/leaf_event_v1.h
engine/events/include/leaf/events/...
engine/events/src/{decision_reducer,event_registry,event_arena,
                   event_trace,subscription,event_runtime}.cpp
```

## 4. Public API (engine)

```cpp
event_runtime rt;
rt.subscribe_notification(id, cb, priority, owner_mod);
rt.subscribe_decision(id, cb, priority, owner_mod);
rt.publish_notification(id, payload);
rt.publish_decision(id, payload) -> decision_result;
rt.unsubscribe_owner(mod_id);
```

C ABI (`LeafApiV1::subscribe_event`) is wired through `runtime_api` for
notification **and** decision events. Decision listeners invoked via the C
ABI currently always vote `PASS` (observe-only); a dedicated vote callback
ABI can be added later without breaking v1.

Core player events include `LEAF_EVENT_PLAYER_CHAT` (`0x0103`, decision)
with `LeafPlayerChatPayloadV1` (player + 256-byte UTF-8 message).

## 5. Packet layout

```text
[LeafEventHeaderV1 / event_header]
[fixed POD payload]
[optional variable bytes]
```

Listeners receive an immutable `event_packet_view` valid only for the
callback duration. Do not retain interior pointers.

## 6. Threading (v1)

- `publish_*` is for `main_sync` descriptors only.
- Wrong/async models return `not_supported` (Scheduler module later).
- No silent thread hops.

## 7. Mod unload

`unsubscribe_owner(mod_id)` removes all listeners and dynamic events for that
owner. `mod_engine` owns `event_runtime` and attaches it to `runtime_api`.

Deferred `dlclose` during active dispatch epoch is planned with Scheduler;
v1 tests cover registry/snapshot safety.

## 8. Intentionally deferred

- Async decision barriers / deadlines
- Hot path specialized dispatcher for tick/entity
- YAML→codegen for schemas
- Kotlin/Native event views
- Network/distributed events

## 9. Next module

**Scheduler** (`main` / `async` / `delay`) so Bridge and Event Bus can
express thread ownership without overloading the bus.
