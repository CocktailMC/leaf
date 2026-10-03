# Module 2 — Native Package Load & Lifecycle

## 1. Responsibilities

Extend `engine/loader` so a resolved `.leafmod` becomes a running native mod:

```text
RESOLVED
  → locate native/<host_target>/lib{id}.so|.dll|.dylib
  → dlopen / LoadLibrary
  → resolve leaf_mod_entry
  → LOADED
  → on_load  → INITIALIZED
  → on_enable → ENABLED
  → on_disable → DISABLED
  → on_unload + dlclose → UNLOADED
```

## 2. Directory structure

```text
engine/loader/include/leaf/loader/
  host_target.hpp
  shared_library.hpp
  native_path.hpp
  runtime_api.hpp
  mod_instance.hpp
  mod_engine.hpp
```

## 3. Public API

```cpp
leaf::current_host_target()
leaf::native_library_filename(mod_id)
leaf::shared_library::open(path)
leaf::mod_instance::load(package, api, host_target)
leaf::mod_engine::bootstrap / enable_all / disable_all / unload_all
```

## 4. LeafApiV1 host (v1.53 additive)

`runtime_api` exposes a real `LeafApiV1` table (additive since Module 2):

| Slot | Behaviour |
|------|-----------|
| `log` | configurable sink (default stderr) |
| `has_capability` | engine capability set |
| `get_server` | attached `minecraft_abi` |
| `send_player_message` / `broadcast_message` | Minecraft ABI (+ live FFM hooks) |
| `player_count` / `get_player_at` / `get_player_name` | online player registry |
| inventory / world / pos / health / food / gamemode / xp / look | stub or live FFM |
| `play_sound` | registry sound at player or world coords |
| `send_actionbar` / `send_title` | HUD overlays |
| `kick_player` | disconnect with reason |
| `give_item` | insert into first suitable inventory slot |
| `apply_effect` / `clear_effects` | potion effects |
| `spawn_particle` | simple registry particles in a dimension |
| `get_world_time` / `set_world_time` | day time ticks per dimension |
| `get_player_velocity` / `set_player_velocity` | blocks/tick motion |
| `get_player_flags` | sneak/sprint/swim/fly/on_ground bits |
| `run_command` | console or player command source |
| `clear_inventory` | wipe player inventory (stub + live FFM) |
| `broadcast_actionbar` / `broadcast_title` | HUD to all online players |
| `set_player_flight` | allow_flight + flying (-1 = unchanged) |
| `get_biome` | biome registry id at block coords |
| `get/set_difficulty` | 0 peaceful .. 3 hard |
| `get/set_weather` | clear/rain/thunder (`LEAF_WEATHER_*`) |
| `get_light_level` | block + sky light 0–15 |
| `get_player_latency` | network latency in ms |
| `get/set_world_spawn` | per-dimension spawn block coords |
| `is_player_op` | operator status (0/1) |
| `get_player_uuid` | hyphenated UUID string |
| `get_player_permission_level` | vanilla 0–4 permission level |
| `find_player_by_uuid` | resolve online handle from UUID string |
| `find_player_by_name` | resolve online handle from login name |
| `get/set_block_registry_id` | block registry string at coords |
| `give_item_registry_id` | give item by registry string |
| `get/set_inventory_item_registry_id` | inventory slot by registry string |
| `teleport_player` | pos + dimension + look in one call |
| `get/set_selected_slot` | hotbar selection 0–8 |
| `get/set_held_item_registry_id` | item in selected hotbar slot |
| `get/set_equipment_item_registry_id` | mainhand/offhand/armor (`LEAF_EQUIP_*`) |
| `get_world_seed` | world generation seed per dimension |
| `heal_player` | restore health to max |
| `get/set_player_absorption` | absorption hearts |
| `get/set_player_invulnerable` | damage immunity flag 0/1 |
| `get/set_player_air` | underwater air ticks |
| `get/set_player_fire_ticks` | remaining on-fire ticks |
| `get/set_player_frozen_ticks` | powdered-snow frozen ticks |
| `extinguish_player` / `unfreeze_player` | clear fire / freeze |
| `get/set_player_no_gravity` | entity no-gravity flag 0/1 |
| `get/set_player_silent` | entity silent flag 0/1 |
| `get/set_player_glowing` | entity glowing flag 0/1 |
| `get/set_player_invisible` | entity invisible flag 0/1 |
| `get/set_player_portal_cooldown` | entity portal cooldown ticks |
| `get_player_max_air` | max air ticks (typically 300) |
| `refill_player_air` | restore air to max |
| `is_player_alive` | health > 0 → 0/1 |
| `subscribe_event` / `unsubscribe_event` / `subscribe_decision` | event bus |
| `post_main` / `delay_main` / `cancel_task` | scheduler |

## 5. Thread safety

- Bootstrap / enable / disable / unload are **single-threaded** (engine bootstrap thread).
- `runtime_api` log/capability accessors are mutex-guarded for concurrent log calls from mod worker threads later.
- Do not `dlclose` while mod code may still run on another thread (future scheduler will enforce this).

## 6. Lifecycle / error handling

- Mod hook exceptions are caught at the boundary → `mod_load_failed` + `FAILED` state.
- Missing library / missing entry symbol → `mod_load_failed` / `mod_entry_missing`.
- Entry id must match manifest id when provided.
- Disable/unload walk mods in **reverse** dependency order.

## 7. Cross-version / platform

Host target triples:

```text
linux-x86_64 | linux-aarch64 | windows-x86_64 | macos-arm64 | …
```

Library naming:

```text
Linux/macOS: lib{id}.so / lib{id}.dylib
Windows:     {id}.dll
```

Phase-1 platforms remain Windows/Linux x86_64; other triples are detected for forward compatibility.

## 8. Next module

**Event Bus** — replace subscribe/unsubscribe stubs; Bridge will forward Fabric/Forge/NeoForge events into unified Leaf events.
