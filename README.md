# LEAFMC

Native cross-loader, cross-version Minecraft mod runtime powered by **C++23**,
with first-class **C++23** and **Kotlin/Native** mod development.

> Forge / Fabric / NeoForge are thin Minecraft bridges.  
> **LEAFMC** is the real mod loader and native runtime.  
> Leaf Mods only depend on the Leaf API / ABI — never on `net.minecraft.*`
> or loader packages.

## Status

**Module 0–8** — Core → three-loader runtime + zip leafmods + leaf-cli  
**Live Fabric + NeoForge** — `runServer` loads `greeter.leafmod` on MC 1.21.1  
**LeafApiV1.58** — is_player_alive + refill_player_air + get_player_max_air + get/set_player_portal_cooldown + get/set_player_invisible + get/set_player_glowing + get/set_player_silent + get/set_player_no_gravity + extinguish_player + unfreeze_player + get/set_player_frozen_ticks + get/set_player_fire_ticks + get/set_player_air + get/set_player_invulnerable + heal_player + get/set_player_absorption + get_world_seed + get/set_equipment_item_registry_id + get/set_held_item_registry_id + get/set_selected_slot + teleport_player + get/set_inventory_item_registry_id + give_item_registry_id + get/set_block_registry_id + find_player_by_name + find_player_by_uuid + get_player_permission_level + get_player_uuid + is_player_op + get/set_world_spawn + get_player_latency + get_light_level + get/set_weather + get/set_difficulty + get_biome + set_player_flight + broadcast HUD + clear_inventory + run_command + player flags + velocity + world time + particles/effects/give/kick/HUD/sound + look/XP/gamemode/food/health/pos/enum, scheduler, inventory/world, rich events  
**Next** — ForgeGradle live resolve · KN leaf_mod_entry · richer item components

| Piece | State |
|-------|-------|
| Offline 3-loader harness + greeter.leafmod | done |
| Drop-in install / `install-to-game.sh` | done |
| Zip + directory `*.leafmod` / `*.leaf` | done |
| Fabric Loom live (`bridge/fabric-live`) | done (`runServer` verified) |
| NeoForge userdev live (`bridge/neoforge-live`) | done (`runServer` verified) |
| Chat + broadcast + inventory + multi-dim world FFM | done |
| Player get/set position FFM | done |
| UUID handles + `register_player` names | done |
| Player chat + death + block break/place decisions | done |
| `LeafItemStackV1` + `LEAF_CAP_INVENTORY_STACK_V1` | done |
| Kotlin/Native SDK scaffold | done (cinterop sources) |
| Forge live MDK | sources parity; FG6 resolve deferred (use NeoForge) |

## Architecture (target)

```text
Leaf Mod (C++23 | Kotlin/Native)
          │
      Leaf SDK
          │
      Leaf C ABI
          │
┌─────────────────────────────┐
│      LEAFMC C++23 Engine    │
│ Mod Manager · Events · …    │
└─────────────┬───────────────┘
              │
      Minecraft ABI Layer
              │
   Fabric / Forge / NeoForge Bridge
              │
          Minecraft
```

## Build

Requirements:

- CMake ≥ 3.28
- C++23 compiler (GCC 13+ / Clang 17+ / MSVC recent)
- Ninja (recommended)
- JDK 25 (FFM bridge / harness; `LEAF_JDK` or `~/.local/jdk25`)

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DLEAF_BUILD_EXAMPLES=ON
cmake --build build
ctest --test-dir build --output-on-failure

# Three-loader offline smoke (loads greeter.leafmod via Fabric/Forge/NeoForge paths)
./scripts/smoke-three-loaders.sh

# Assemble drop-in mods/ + leafmods/ tree
./scripts/assemble-install.sh
```

Bridge JARs (stub-compile, Gradle 9 + JDK 25):

```bash
cd bridge && ./gradlew jar
```

## Layout

```text
LEAFMC/
├── abi/           # Stable C ABI headers (mod boundary)
├── engine/        # C++23 runtime (loader, events, handles, …)
├── sdk/           # Language SDKs wrapping the C ABI
├── bridge/        # Thin Fabric / Forge / NeoForge bootstraps
├── minecraft/     # Per-version ABI, mappings, symbols
├── tooling/       # CLI, packager, installer
├── examples/      # Sample .leafmod projects
└── tests/         # Engine unit tests
```

## Design rules (non-negotiable)

1. Leaf Mods must not depend on Minecraft or loader Java packages.
2. Plugin boundaries use **pure C ABI** — no STL / exceptions / RTTI across the boundary.
3. Game objects are exposed as **typed handles**, not Java references.
4. Version / loader differences are expressed as **capabilities**, not `if (fabric)`.
5. Bridges only bootstrap the engine; they never load `.leafmod` files.

## License

TBD.
