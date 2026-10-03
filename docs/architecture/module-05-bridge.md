# Module 5 — Multi-Loader Bridge

## Principle

```text
Forge / Fabric / NeoForge = thin Minecraft Bridge
LEAFMC                    = real Mod Loader + Runtime
```

Bridges **must not**:

- scan `leafmods/`
- resolve `.leafmod` dependencies
- own Leaf Mod lifecycle / API / scheduler

Bridges **may**:

- load `libleaf_bridge`
- call `leaf_bridge_init` / `pump_main` / lifecycle forwards
- translate loader events → `leaf_bridge_emit_*`

## Native ABI

`abi/include/leaf/abi/leaf_bridge_v1.h`

Shared library: `libleaf_bridge.so` (`engine/bridge-host`)

Preferred entry for FFM: `leaf_bridge_init_flat` (flat string/int args, no struct layout).

## Java layout (FFM, no JNI)

```text
bridge/
├── common/     LeafBridge.java (java.lang.foreign downcalls)
│               + smoke/BridgeSmoke.java
├── stubs/      compile-only Fabric/Forge/NeoForge façades
├── fabric/     leaf-bootstrap-fabric
├── forge/      leaf_bootstrap_forge
└── neoforge/   leaf_bootstrap_neoforge
```

`LeafBridge` binds the native host via the Foreign Function & Memory API
(JDK 22+; CI uses JDK 25). There is **no JNI**.

Select the shared library with:

```text
-Dleaf.bridge.library=/absolute/path/to/libleaf_bridge.so
```

Smoke:

```bash
javac --release 25 -d out \
  bridge/common/src/main/java/dev/leafmc/bridge/LeafBridge.java \
  bridge/common/src/main/java/dev/leafmc/bridge/smoke/BridgeSmoke.java
java --enable-native-access=ALL-UNNAMED \
  -Dleaf.bridge.library=$PWD/build/engine/bridge-host/libleaf_bridge.so \
  -cp out dev.leafmc.bridge.smoke.BridgeSmoke
```

Production builds replace `stubs` with real loader tooling (Loom / FG / NeoGradle).

## Minecraft ABI attachment

On `leaf_bridge_init*`, the host:

1. creates a version/loader `minecraft_abi` (stub today)
2. attaches it to `runtime_api` so Leaf ABI calls (`get_server`,
   `send_player_message`, …) route through the ABI layer
3. optionally bootstraps / enables `.leafmod`s

See `docs/architecture/module-06-minecraft-abi.md`.

## Main thread

`leaf_bridge_pump_main` binds/validates the Minecraft main thread and drains
the scheduler. Canonical `main_sync` events should be emitted on that thread.
