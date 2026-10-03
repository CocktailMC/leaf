# Three-loader runtime (stub-compile + offline harness)

## Goal

Drop a thin bootstrap JAR into Fabric / Forge / NeoForge `mods/`, point
`leafmods/` at `*.leafmod` packages, and let LEAFMC load native Leaf Mods.

Bridges **never** scan leafmods — `libleaf_bridge` does.

## Offline verification (no Minecraft)

```bash
./scripts/smoke-three-loaders.sh
```

This:

1. Builds `libleaf_bridge.so` + Greeter example
2. Packs `dist/leafmods/greeter.leafmod`
3. Runs `MultiLoaderHarness` through Fabric / Forge / NeoForge init paths

## Game install layout

```text
.minecraft/          (or server root)
├── mods/
│   └── leaf-bootstrap-<loader>-0.1.0.jar
├── leafmods/
│   └── greeter.leafmod/
│       ├── leaf.mod.json
│       └── native/<host>/libgreeter.so
└── (optional) config
```

System properties (or defaults):

| Property | Default |
|----------|---------|
| `leaf.bridge.library` | extract from JAR `native/<triple>/…` |
| `leaf.leafmods.dir` | `leafmods` |
| `leaf.minecraft.version` | `1.21.1` |
| `leaf.game.dir` | unset |
| `leaf.autoload` | `true` |

## leaf-cli

```bash
python3 tooling/leaf-cli/leaf.py pack \
  --manifest examples/cpp23-mod/leaf.mod.json \
  --native build/examples/cpp23-mod/libgreeter.so \
  --out dist/leafmods/greeter.leafmod

python3 tooling/leaf-cli/leaf.py pack ... --out dist/greeter.leafmod --zip
python3 tooling/leaf-cli/leaf.py install dist/greeter.leafmod --leafmods ~/.minecraft/leafmods
python3 tooling/leaf-cli/leaf.py install dist/sample.leaf --leafmods ~/.minecraft/leafmods
python3 tooling/leaf-cli/leaf.py list --leafmods ~/.minecraft/leafmods
```

Zip `.leafmod` / `.leaf` archives are auto-extracted under `leafmods/.leafmc-extract/` on load.

## Gradle bridge JARs

```bash
cd bridge && ./gradlew jar
./scripts/assemble-install.sh   # embeds native + greeter into dist/install/
```

Stub-compile only until Loom / FG / NeoGradle are wired against a live game.

## Install into a game root

```bash
./scripts/install-to-game.sh /path/to/minecraft fabric
./scripts/install-to-game.sh /path/to/server forge
```
