# Kotlin/Native Leaf SDK (scaffold)

Target: cinterop against `abi/include/leaf/abi/*.h`, mirroring `sdk/cpp`.

## Layout

```text
sdk/kotlin-native/
├── leaf.def
├── build.gradle.kts
├── src/leafmc/sdk/
│   ├── Api.kt
│   ├── Mod.kt
│   └── Events.kt
└── README.md
```

## Build (when KN toolchain is available)

```bash
cd sdk/kotlin-native
./gradlew linkDebugSharedNative   # or linkReleaseSharedNative
```

Package the produced `libleaf_kn_sdk.so` (or a mod shared lib that links it)
as `native/<triple>/lib<id>.so` inside a `.leafmod`, same as C++ mods.

## Entry

Export `leaf_mod_entry` from a KN dynamic library. Until the KN toolchain is
installed in CI, use the C++23 SDK (`#include <leaf/sdk/sdk.hpp>`).

## Status

Scaffold only — sources and Gradle/cinterop wiring land here; full
`leaf_mod_entry` export + packaging is next once a KN compiler is present.
