# Fabric Loom live bridge (1.21.1)

Produces a remapped Fabric mod that talks to `libleaf_bridge` via FFM and loads
`*.leafmod` packages from `leafmods/`.

## Build

```bash
# Engine + greeter first
cmake -S ../.. -B ../../build -G Ninja -DLEAF_BUILD_EXAMPLES=ON
cmake --build ../../build --target leaf_bridge leaf_example_greeter

cd bridge/fabric-live
JAVA_HOME=$HOME/.local/jdk25 ./gradlew remapJar
```

Output: `build/libs/leaf-bootstrap-fabric-live-0.1.0.jar`

## Run (Loom)

```bash
# Embed or point at the native host
export LEAF_BRIDGE_LIBRARY=$PWD/../../build/engine/bridge-host/libleaf_bridge.so

JAVA_HOME=$HOME/.local/jdk25 ./gradlew runServer \
  -Dleaf.bridge.library=$LEAF_BRIDGE_LIBRARY \
  -Dleaf.leafmods.dir=$PWD/../../dist/leafmods
```

Requires **JDK 22+** at runtime (FFM). Bytecode is emitted as Java 22 for Loom remapper compatibility.

## Install into an existing Fabric 1.21.1 instance

```bash
./scripts/assemble-install.sh
# Prefer the live remapped jar when available:
cp bridge/fabric-live/build/libs/leaf-bootstrap-fabric-live-0.1.0.jar \
   /path/to/minecraft/mods/
# + leafmods/greeter.leafmod from dist/install
```
