# Module 1 — Mod Discovery & Dependency Resolution

## 1. Responsibilities

`engine/loader` owns everything about finding and ordering Leaf Mods **before**
any native library is `dlopen`'d:

- Scan `leafmods/*.leafmod/` (and `*.leaf` alias) directory packages **and** zip archives
- Parse `leaf.mod.json`
- Validate id / ABI / Minecraft range / host target
- Resolve required & optional dependencies
- Detect duplicates and cycles
- Emit topological load order (`mod_state::resolved`)
- **Skip** individual broken packages (warn to stderr) so other mods still load

Forge / Fabric / NeoForge still do **not** participate.

## 2. Directory structure

```text
engine/loader/
├── include/leaf/loader/
│   ├── mod_state.hpp
│   ├── version_range.hpp
│   ├── manifest.hpp
│   ├── manifest_parser.hpp
│   ├── discovery.hpp
│   ├── dependency_graph.hpp
│   └── loader.hpp
└── src/
    ├── json_lite.hpp          # internal minimal JSON parser
    ├── version_range.cpp
    ├── manifest_parser.cpp
    ├── discovery.cpp
    └── dependency_graph.cpp
```

## 3. Public API

```cpp
leaf::parse_mod_manifest(json_text) -> result<mod_manifest>
leaf::load_mod_manifest(path)       -> result<mod_manifest>
leaf::discover_mods(leafmods_dir)   -> result<vector<discovered_mod>>
leaf::resolve_mods(mods, options)   -> result<resolve_report>
```

`resolve_options` carries the live Minecraft version and host target triple.

## 4. Manifest / ABI notes

Phase-1 package layout (directory only):

```text
example.leafmod/
└── leaf.mod.json
```

Zip `.leafmod` archives and native library loading are **Module 2**.

Dependency entries accept:

```json
"economy"
{ "id": "economy", "version": ">=1.0.0" }
```

## 5. Thread safety

Discovery / parse / resolve are synchronous and not internally locked.
Call them from a single engine bootstrap thread (Bridge → Engine start).
Later concurrent hot-reload will add its own synchronization.

## 6. Lifecycle

```text
DISCOVERED  --resolve_mods-->  RESOLVED
```

Failed validation leaves mods out of the load order and returns
`leaf::error` (`mod_duplicate_id`, `mod_dependency_unsatisfied`,
`mod_dependency_cycle`, `mod_version_incompatible`, `abi_version_mismatch`, …).

## 7. Error handling

All APIs return `leaf::result<T>`. JSON / IO failures map to
`mod_manifest_invalid`, `path_not_found`, or `io_error`.

## 8. Cross-version compatibility

Minecraft constraints use `min` / `max` with optional `x` wildcards
(`1.21.x`). Host binaries are filtered by `targets` (e.g. `linux-x86_64`).
Leaf ABI major must equal `leaf_abi_version_major` (currently 1).

## 9. Next module

**Native package load**: locate `native/<target>/` shared libraries,
`dlopen` / `LoadLibrary`, resolve `leaf_mod_entry`, drive
LOADED → INITIALIZED → ENABLED lifecycle.
