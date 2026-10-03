# LEAFMC progress snapshot

Verified locally (**C++ tests**, Fabric + NeoForge live smoke):

- **LeafApiV1.58** — `is_player_alive` + `refill_player_air` + `get_player_max_air` +
  `get/set_player_portal_cooldown` +
  `get/set_player_invisible` +
  `get/set_player_glowing` + `get/set_player_silent` + `get/set_player_no_gravity` +
  extinguish/unfreeze + frozen/fire/air/invulnerable + heal/absorption + world_seed +
  equipment/held/selected_slot/teleport + inventory/item/block registry +
  find/permission/uuid/op + world spawn/latency/light/weather/difficulty/biome/
  flight + broadcast HUD + clear_inventory + run_command + flags/velocity/time/
  particles/effects/give/kick/HUD/sound + look/XP/gamemode/food/health/pos.
- **Events** — join/leave/chat/death, entity spawn/remove, world load,
  block break/place, server_tick on `pump_main`.
- **Live FFM** — max_air + portal_cooldown + invisible + glowing + silent + no_gravity +
  frozen_ticks + fire_ticks + air + invulnerable + absorption + world_seed +
  selected_slot + inventory(+registry)/world/pos/teleport/stats/look/sound/HUD/kick/
  give(+registry)/effects/particles/time/velocity/flags/commands/
  clear_inventory/flight/biome/difficulty/weather/light/latency/spawn/op/
  uuid/permission/find/block-registry
  (heal/refill_air/is_player_alive/held/equipment compose existing hooks).
- **Discovery / CLI** — `*.leafmod` and `*.leaf`; live/install trees sync
  `greeter.leaf` (one package name only — duplicate ids are rejected).
- **Loaders** — Fabric + NeoForge live `runServer` load leaf packages;
  Forge live sources in parity (FG6 needs JDK 21; workspace has JDK 25).
  Offline assemble installs all three bootstrap JARs.
- **Tooling** — `leaf doctor`, `leaf verify`, `sync-live` (always repacks greeter),
  `assemble-install`.

```bash
./scripts/smoke-three-loaders.sh
./scripts/live-runserver-smoke.sh fabric|neoforge
python3 tooling/leaf-cli/leaf.py doctor
python3 tooling/leaf-cli/leaf.py verify dist/leafmods/greeter.leafmod
./scripts/assemble-install.sh
```

Open: ForgeGradle live resolve · KN `leaf_mod_entry` · data-component inventory blobs.
