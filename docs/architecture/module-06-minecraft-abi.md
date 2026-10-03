# Module 6 — Minecraft ABI Layer

## Role

```text
Leaf Mod  →  Leaf C ABI  →  runtime_api  →  minecraft_abi  →  game
```

Leaf Mods never see `net.minecraft.*`. Version and loader differences live
behind `leaf::minecraft_abi`.

## Interface

`minecraft/abi/include/leaf/minecraft/minecraft_abi.hpp`

Capabilities:

- `minecraft_version()` / `loader()` / `capabilities()`
- `get_server()`
- `send_player_message(player, message)`
- `broadcast_message(message)`
- `player_count()`
- `player_name(player)`
- `get_inventory_slot` / `set_inventory_slot` / stack variants
- `get_block` / `set_block` / dim variants
- `get_player_pos` / `set_player_pos` (stub + live FFM)
- `get_player_look` / `set_player_look` (stub + live FFM)
- `play_sound` (registry id; player or world coords; stub + live FFM)
- `send_actionbar` / `send_title` (HUD overlays; stub + live FFM)
- `kick_player` (disconnect with reason; stub + live FFM)
- `give_item` (merge/empty-slot insert; stub + live FFM)
- `apply_effect` / `clear_effects` (potion effects; stub + live FFM)
- `spawn_particle` (simple registry particles; stub + live FFM)
- `get_world_time` / `set_world_time` (day time; stub + live FFM)
- `get_player_velocity` / `set_player_velocity` (motion; stub + live FFM)
- `get_player_flags` (sneak/sprint/swim/fly/ground; stub + live FFM)
- `run_command` (console or player source; stub + live FFM)
- `clear_inventory` (stub + live FFM)
- `broadcast_actionbar` / `broadcast_title` (fan-out via per-player HUD hooks)
- `set_player_flight` (allow + flying; stub + live FFM)
- `get_biome` (registry id string; stub + live FFM)
- `get/set_difficulty` (0–3; stub + live FFM)
- `get/set_weather` (`LEAF_WEATHER_*`; stub + live FFM)
- `get_light_level` (block + sky; stub + live FFM)
- `get_player_latency` (ms; stub + live FFM)
- `get/set_world_spawn` (per-dimension; stub + live FFM)
- `is_player_op` (0/1; stub + live FFM)
- `get_player_uuid` (hyphenated string; stub + live FFM)
- `get_player_permission_level` (0–4; stub + live FFM)
- `find_player_by_uuid` (online handle lookup; stub + live FFM)
- `find_player_by_name` (exact/case-insensitive name; stub + live FFM)
- `get/set_block_registry_id` (registry string; stub + live FFM)
- `give_item_registry_id` (item registry string; stub + live FFM)
- `get/set_inventory_item_registry_id` (slot registry string; stub + live FFM)
- `teleport_player` (pos + look; stub + live FFM)
- `get/set_selected_slot` (hotbar 0–8; stub + live FFM)
- `get/set_held_item_registry_id` (selected hotbar item; composes slot + inventory)
- `get/set_equipment_item_registry_id` (LEAF_EQUIP_*; slots 36–40 + mainhand)
- `get_world_seed` (generation seed; stub + live FFM)
- `heal_player` (restore to max health; composes health hooks)
- `get/set_player_absorption` (absorption hearts; stub + live FFM)
- `get/set_player_invulnerable` (damage immunity; stub + live FFM)
- `get/set_player_air` (underwater air ticks; stub + live FFM)
- `get/set_player_fire_ticks` (on-fire ticks; stub + live FFM)
- `get/set_player_frozen_ticks` (powdered-snow frozen ticks; stub + live FFM)
- `extinguish_player` / `unfreeze_player` (compose fire/frozen ticks to 0)
- `get/set_player_no_gravity` (Entity no-gravity 0/1; stub + live FFM)
- `get/set_player_silent` (Entity silent 0/1; stub + live FFM)
- `get/set_player_glowing` (Entity glowing 0/1; stub + live FFM)
- `get/set_player_invisible` (Entity invisible 0/1; stub + live FFM)
- `get/set_player_portal_cooldown` (Entity portal cooldown ticks; stub + live FFM)
- `get_player_max_air` (Entity max air ticks; stub + live FFM)
- `refill_player_air` (restore to max air; composes max_air + set_player_air)
- `is_player_alive` (health > 0 → 0/1; composes get_player_health)

Factory: `create_minecraft_abi(version, loader_kind)`.

## Stub backend (current)

`stub_minecraft_abi` records messages, players, inventory slots, positions,
velocities, flags, sounds, HUD text, kicks, effects, particles, world time,
commands, and block ids in-process so engine and bridge tests run without a live
Minecraft JVM. Live bridges install FFM upcalls for chat/broadcast/inventory/
world/player pos/look/sound/HUD/kick/give/effects/particles/time/velocity/flags/
commands/op/uuid/permission/find-by-uuid/name/block-registry/item-registry/
selected_slot and call `leaf_bridge_register_player` so names stay accurate.
Live also installs `get_world_seed`, `player_absorption`,
`player_invulnerable`, `player_air`, `player_fire_ticks`,
`player_frozen_ticks`, `player_no_gravity`, `player_silent`,
`player_glowing`, `player_invisible`, `player_portal_cooldown`,
and `get_player_max_air` FFM (`refill_player_air` composes those hooks;
`is_player_alive` composes health).

Capability defaults:

| MC version | Caps |
|------------|------|
| ≥ 1.20.4   | `data_components`, `registry_modern` |
| ≤ 1.20.1   | `legacy_item_nbt`, `registry_legacy` |
| ≥ 1.20     | `custom_payload_v2` |

## Version backends (next)

```text
minecraft/abi/versions/
├── 1_20_1/   # real Minecraft symbols (placeholder)
└── 1_21_1/
```

Real backends will bind Mojmap / intermediary / SRG symbols and replace the
stub when the bridge reports a matching game version.

## Wiring

`bridge_host::init` creates the ABI and calls
`runtime_api::attach_minecraft_abi`. `runtime_api` ABI entry points on
`LeafApiV1` forward to the attached implementation.
