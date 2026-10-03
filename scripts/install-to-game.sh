#!/usr/bin/env bash
# Copy dist/install into a Minecraft / server root.
# Usage: ./scripts/install-to-game.sh /path/to/game/root [fabric|forge|neoforge]
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GAME="${1:-}"
LOADER="${2:-fabric}"

if [[ -z "$GAME" ]]; then
  echo "usage: $0 <game-root> [fabric|forge|neoforge]" >&2
  exit 1
fi

case "$LOADER" in
  fabric|forge|neoforge) ;;
  *) echo "loader must be fabric|forge|neoforge" >&2; exit 1 ;;
esac

"$ROOT/scripts/assemble-install.sh" >/dev/null

mkdir -p "$GAME/mods" "$GAME/leafmods"
cp -f "$ROOT/dist/install/mods/leaf-bootstrap-${LOADER}-0.1.0.jar" "$GAME/mods/"
rm -rf "$GAME/leafmods/greeter.leafmod" "$GAME/leafmods/greeter.leaf"
if [[ -d "$ROOT/dist/install/leafmods/greeter.leaf" ]]; then
  cp -a "$ROOT/dist/install/leafmods/greeter.leaf" "$GAME/leafmods/"
elif [[ -d "$ROOT/dist/install/leafmods/greeter.leafmod" ]]; then
  cp -a "$ROOT/dist/install/leafmods/greeter.leafmod" "$GAME/leafmods/greeter.leaf"
fi

echo "Installed LEAFMC ${LOADER} bridge + greeter.leaf into $GAME"
echo "  mods/leaf-bootstrap-${LOADER}-0.1.0.jar"
echo "  leafmods/greeter.leaf/"
echo "Remember: JVM --enable-native-access=ALL-UNNAMED (JDK 22+)"
