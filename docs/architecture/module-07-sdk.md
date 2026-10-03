# Module 7 — Leaf Language SDKs

## Principle

```text
Leaf Mod  →  Leaf SDK (C++23 | Kotlin/Native)  →  Leaf C ABI  →  Engine
```

Mods **must not** include `engine/` headers or Minecraft / loader packages.
The SDK is a thin, header-only (C++) / cinterop (KN) façade over
`abi/include/leaf/abi/*.h`.

## C++23 SDK

```text
sdk/cpp/include/leaf/sdk/
├── sdk.hpp      umbrella
├── status.hpp   LeafStatus wrapper
├── api.hpp      leaf::sdk::api + subscription
├── events.hpp   event_view + typed payload helpers
└── mod.hpp      leaf::sdk::mod + LEAF_DEFINE_MOD
```

CMake target: `leaf::sdk` (INTERFACE).

### Minimal mod

```cpp
#include <leaf/sdk/sdk.hpp>

class hello final : public leaf::sdk::mod {
public:
    leaf::sdk::mod_info info() const noexcept override {
        return {.id = "hello", .name = "Hello", .version = "1.0.0"};
    }
    void on_enable(leaf::sdk::api& a) override {
        a.info("hello enabled");
        a.subscribe_method<&hello::on_join>(LEAF_EVENT_PLAYER_JOIN, this, sub_);
    }
private:
    void on_join(leaf::sdk::event_view view) {
        auto ev = leaf::sdk::as_player_join(view);
        (void)ev;
    }
    leaf::sdk::subscription sub_;
};

LEAF_DEFINE_MOD(hello)
```

### Example

`examples/cpp23-mod` — Greeter mod (`id = greeter`).

```bash
cmake -S . -B build -G Ninja -DLEAF_BUILD_EXAMPLES=ON
cmake --build build --target leaf_example_greeter
```

## Kotlin/Native

Scaffold under `sdk/kotlin-native/` — `leaf.def` cinterop + `Api`/`Mod`/`Events`
Kotlin sources + Gradle multiplatform stub. Full `leaf_mod_entry` export
requires a KN toolchain.

## Safety notes

- `api::log` / `send_player_message` / `broadcast_message` require NUL-terminated
  C strings (string literals are fine; arbitrary `string_view` slices are not).
- `event_view` is valid only inside the listener callback.
- `as_player_chat` exposes a pointer into the event payload — do not retain it.
- `subscription` unsubscribes on destroy; call `release()` if the engine
  should keep the listener across an SDK object teardown.
- `LEAF_DEFINE_MOD` must appear in exactly one TU per shared library.
- Rebuild `.leafmod` natives whenever `LeafApiV1` grows (ABI minor bump).