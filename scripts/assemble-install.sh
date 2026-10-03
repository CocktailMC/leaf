#!/usr/bin/env bash
# Assemble installable bridge JARs (with embedded native) + greeter.leafmod.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

JDK="${LEAF_JDK:-$HOME/.local/jdk25}"
export JAVA_HOME="${JAVA_HOME:-$JDK}"

./scripts/smoke-three-loaders.sh

ARCH="$(uname -m)"
case "$ARCH" in
  x86_64|amd64) ARCH=x86_64 ;;
  aarch64|arm64) ARCH=aarch64 ;;
esac
NATIVE_TRIPLE="linux-${ARCH}"
if [[ "$(uname -s)" == "Darwin" ]]; then
  NATIVE_TRIPLE="macos-${ARCH}"
  [[ "$ARCH" == "aarch64" ]] && NATIVE_TRIPLE="macos-arm64"
fi

(
  cd bridge
  ./gradlew jar -q
)

DIST="$ROOT/dist"
INSTALL="$DIST/install"
rm -rf "$INSTALL"
mkdir -p "$INSTALL/mods" "$INSTALL/leafmods"

# Prefer the short *.leaf alias in the install tree (same layout as *.leafmod).
rm -rf "$INSTALL/leafmods/greeter.leafmod" "$INSTALL/leafmods/greeter.leaf"
cp -a "$DIST/leafmods/greeter.leafmod" "$INSTALL/leafmods/greeter.leaf"
cp -f "$ROOT/docs/examples/leaf.json.example" "$INSTALL/leaf.json.example" 2>/dev/null \
  || printf '%s\n' '{"minecraftVersion":"1.21.1","leafmodsDir":"leafmods","autoLoadMods":true}' \
     > "$INSTALL/leaf.json.example"

embed_native_into_jar() {
  local jar="$1"
  local tmp
  tmp="$(mktemp -d)"
  mkdir -p "$tmp/native/$NATIVE_TRIPLE"
  cp -f "$DIST/native/libleaf_bridge.so" "$tmp/native/$NATIVE_TRIPLE/"
  (
    cd "$tmp"
    "$JDK/bin/jar" uf "$jar" "native/$NATIVE_TRIPLE/libleaf_bridge.so"
  )
  rm -rf "$tmp"
}

for loader in fabric forge neoforge; do
  src="bridge/${loader}/build/libs/leaf-bootstrap-${loader}-0.1.0.jar"
  dst="$INSTALL/mods/leaf-bootstrap-${loader}-0.1.0.jar"
  cp -f "$src" "$dst"
  embed_native_into_jar "$dst"
  echo "installed $dst"
done

cat > "$INSTALL/README.txt" <<EOF
LEAFMC drop-in layout
=====================

Copy ONE bootstrap jar for your loader into the game mods/ folder, and copy
the leafmods/ directory next to the game root (or pass -Dleaf.leafmods.dir).

  mods/leaf-bootstrap-fabric-0.1.0.jar
  mods/leaf-bootstrap-forge-0.1.0.jar
  mods/leaf-bootstrap-neoforge-0.1.0.jar
  leafmods/greeter.leaf/

Launch JVM flags:
  --enable-native-access=ALL-UNNAMED
  -Dleaf.minecraft.version=1.21.1   (optional)

Native host is embedded at native/<triple>/libleaf_bridge.so inside each JAR.
Verify offline with: ./scripts/smoke-three-loaders.sh
EOF

echo "install tree:"
find "$INSTALL" -type f | sort
echo "assemble-install: OK"
