# Module 0 — Engine Core Foundation

## 1. Responsibilities

This module establishes the non-negotiable substrate for everything else:

- Stable error taxonomy (`leaf::error_code` / `leaf::error`)
- `std::expected`-based `leaf::result<T>`
- Engine / ABI version helpers
- Capability set (loader-agnostic feature detection)
- Typed generational handles + `handle_table`
- Leaf C ABI v1 header (pure C)

It does **not** yet load mods, talk to Minecraft, or install JNI hooks.

## 2. Directory structure

```text
abi/include/leaf/abi/leaf_abi_v1.h
engine/core/include/leaf/core/{error,result,version,capability,abi_status,export,core}.hpp
engine/object-table/include/leaf/object/{handle,handle_table,object}.hpp
tests/test_{version,capability,handle_table,abi_header}.cpp
```

## 3. Public C++ API (engine-internal)

```cpp
leaf::result<T>          // std::expected<T, leaf::error>
leaf::version / parse_version(...)
leaf::capability_set
leaf::handle<Tag> / player_handle / ...
leaf::handle_table<T>
leaf::to_abi_status(error_code) -> LeafStatus
```

Mods should prefer the future SDK wrappers, not these headers directly
(except via documented SDK re-exports).

## 4. C ABI (mod boundary)

See `abi/include/leaf/abi/leaf_abi_v1.h`:

- `LeafApiV1` — function table passed into `leaf_mod_entry`
- `LeafHandle` — `uint64_t` opaque id
- `LeafStatus` / `LeafCapability` / `LeafEventId`
- `LeafModExportsV1` — load/enable/disable/unload hooks

## 5. Thread safety

| Component | Safety |
|-----------|--------|
| `error` / `version` / `capability_set` | Value types; caller-owned |
| `handle<Tag>` | Trivially copyable; no shared state |
| `handle_table<T>` | Internally mutex-guarded; safe for concurrent create/get/destroy |

`handle_table::get` returns a `reference_wrapper` only valid while the
caller still holds a live handle **and** has not destroyed it. Callers must
not retain references across `destroy` or unlock and assume stability without
external synchronization relative to destroyers.

## 6. Lifecycle / ownership

- Handle `0` is null.
- Index is 1-based; generation starts at 1.
- `destroy` increments generation and returns the slot to a free list.
- Stale raw bits fail with `error_code::stale_handle`.

Future JNI integration will store `jobject` GlobalRef / WeakGlobalRef
*inside* table values — never hand raw JNI refs to mods.

## 7. Error handling

- Inside C++23 engine: `leaf::result<T>` / `leaf::status`.
- Across C ABI: `LeafStatus` via `to_abi_status`.
- No C++ exceptions across the ABI boundary.

## 8. Cross-version compatibility notes

Capabilities exist so mods write:

```cpp
if (caps.has(leaf::capability::data_components)) { ... }
```

instead of branching on Minecraft 1.20.1 vs 1.21.1 or Fabric vs Forge.
Per-version Minecraft ABI implementations (later modules) populate the set.

## 9. Next module

**Mod discovery + `.leafmod` manifest parser + dependency graph**
(`engine/loader`).
