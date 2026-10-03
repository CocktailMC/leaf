#!/usr/bin/env bash
# Build engine + greeter.leafmod, then run Java three-loader harness.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

JDK="${LEAF_JDK:-$HOME/.local/jdk25}"
if [[ ! -x "$JDK/bin/javac" ]]; then
  echo "JDK not found at $JDK (set LEAF_JDK)" >&2
  exit 1
fi

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DLEAF_BUILD_EXAMPLES=ON
cmake --build build --target leaf_bridge leaf_example_greeter leaf_tests

./build/tests/leaf_tests

DIST="$ROOT/dist"
LEAFMODS="$DIST/leafmods"
mkdir -p "$LEAFMODS" "$DIST/native"

# Detect host triple (mirrors leaf::current_host_target roughly).
ARCH="$(uname -m)"
case "$ARCH" in
  x86_64|amd64) ARCH=x86_64 ;;
  aarch64|arm64) ARCH=aarch64 ;;
esac
TARGET="linux-${ARCH}"
if [[ "$(uname -s)" == "Darwin" ]]; then
  TARGET="macos-${ARCH}"
  [[ "$ARCH" == "aarch64" ]] && TARGET="macos-arm64"
fi

python3 tooling/packager/leaf_pack.py \
  --manifest examples/cpp23-mod/leaf.mod.json \
  --native "build/examples/cpp23-mod/libgreeter.so" \
  --out "$LEAFMODS/greeter.leafmod" \
  --target "$TARGET"

# Keep live loader workspaces on the same greeter binary (ABI must match host).
for live_dir in \
  bridge/fabric-live/run/leafmods \
  bridge/neoforge-live/run/leafmods \
  bridge/neoforge-live/run/server/leafmods
do
  mkdir -p "$live_dir"
  rm -rf "$live_dir/greeter.leafmod" "$live_dir/greeter.leaf"
  cp -a "$LEAFMODS/greeter.leafmod" "$live_dir/greeter.leaf"
done

cp -f build/engine/bridge-host/libleaf_bridge.so "$DIST/native/libleaf_bridge.so"

CLASSES="$DIST/bridge-classes"
rm -rf "$CLASSES"
mkdir -p "$CLASSES"

# Compile stubs + common + loader bootstraps + harness together.
mapfile -t SOURCES < <(find bridge/stubs bridge/common bridge/fabric bridge/forge bridge/neoforge bridge/harness -name '*.java' | sort)
"$JDK/bin/javac" --release 25 -d "$CLASSES" "${SOURCES[@]}"

"$JDK/bin/java" --enable-native-access=ALL-UNNAMED \
  -Dleaf.bridge.library="$DIST/native/libleaf_bridge.so" \
  -Dleaf.leafmods.dir="$LEAFMODS" \
  -Dleaf.minecraft.version=1.21.1 \
  -Dleaf.expect.mod=greeter \
  -cp "$CLASSES" \
  dev.leafmc.bridge.harness.MultiLoaderHarness

echo "smoke-three-loaders: OK"
