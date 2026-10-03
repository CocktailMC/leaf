# Forge live (WIP)

Official ForgeGradle 6 for 1.21.1 does **not** yet support Gradle 9, and its
mapped artifact generation is flaky with JDK 25 toolchains in this workspace.

`LeafForgeLiveBootstrap` is kept in **source parity** with NeoForge live
(UUID handles, inventory/stack/world/pos hooks, chat/death/block decisions).

## Current recommendation for 1.21.1

Use **NeoForge live** (`../neoforge-live`) — verified `runServer` loads
`greeter.leafmod`. NeoForge is the maintained 1.21.x Forge-family loader.

Offline Forge bootstrap JAR (stub-compile) still works via:

```bash
./scripts/assemble-install.sh
./scripts/install-to-game.sh /path/to/game forge
```

## Resume ForgeGradle live later

1. Use Gradle **8.14** wrapper (already configured here).
2. Run FG setup with **JDK 21** as `JAVA_HOME` for Gradle, toolchain 22 for FFM.
3. Prefer depending on a prebuilt `bridge/common` JAR instead of sharing sources
   with release=22 inside FG6.

Sources in `src/main/java/.../LeafForgeLiveBootstrap.java` are ready once FG
mapped Forge resolves.
