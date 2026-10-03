# Fabric Loom live-dev (experimental)

This directory is a **template** for running the LEAFMC Fabric bridge inside a
real Minecraft 1.21.1 Loom workspace.

## Status

The default `bridge/` multi-project still uses compile stubs so offline CI and
`./scripts/smoke-three-loaders.sh` stay deterministic.

For a live game:

1. Create a Fabric Loom workspace (or copy `build.gradle.kts.example`).
2. Depend on `bridge/common` sources / JAR and replace stubs with Fabric API.
3. Place `libleaf_bridge.so` via `-Dleaf.bridge.library=...` or embed under
   `native/<triple>/` in the remapped jar.
4. Put `*.leafmod` packages in `<game>/leafmods/`.
5. Launch with JDK 22+ and `--enable-native-access=ALL-UNNAMED`.

Until Loom is fully wired here, use:

```bash
./scripts/assemble-install.sh
./scripts/install-to-game.sh /path/to/minecraft fabric
```
