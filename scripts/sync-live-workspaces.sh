#!/usr/bin/env bash
# Sync built libleaf_bridge.so + greeter.leafmod into live loader run dirs.
# Always repacks greeter from the latest libgreeter.so so ABI bumps cannot
# leave a stale .leafmod that crashes on LeafApiV1 slot mismatches.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

NATIVE="$ROOT/build/engine/bridge-host/libleaf_bridge.so"
GREETER_SO="$ROOT/build/examples/cpp23-mod/libgreeter.so"
GREETER="$ROOT/dist/leafmods/greeter.leafmod"
MANIFEST="$ROOT/examples/cpp23-mod/leaf.mod.json"

if [[ ! -f "$NATIVE" ]]; then
  echo "missing $NATIVE — build leaf_bridge first" >&2
  exit 1
fi
if [[ ! -f "$GREETER_SO" ]]; then
  echo "missing $GREETER_SO — build leaf_example_greeter first" >&2
  exit 1
fi
if [[ ! -f "$MANIFEST" ]]; then
  echo "missing $MANIFEST" >&2
  exit 1
fi

mkdir -p "$ROOT/dist/leafmods"
python3 "$ROOT/tooling/leaf-cli/leaf.py" pack \
  --manifest "$MANIFEST" \
  --native "$GREETER_SO" \
  --out "$GREETER"

for live in fabric-live neoforge-live; do
  res="$ROOT/bridge/$live/build/resources/main/native/linux-x86_64"
  mkdir -p "$res"
  cp -f "$NATIVE" "$res/libleaf_bridge.so"
done

for d in \
  bridge/fabric-live/run/leafmods \
  bridge/neoforge-live/run/leafmods \
  bridge/neoforge-live/run/server/leafmods
do
  mkdir -p "$ROOT/$d"
  # Prefer the *.leaf alias on live run dirs so discovery of the short
  # extension is exercised end-to-end (same package layout as *.leafmod).
  rm -rf "$ROOT/$d/greeter.leafmod" "$ROOT/$d/greeter.leaf"
  cp -a "$GREETER" "$ROOT/$d/greeter.leaf"
  echo "synced greeter.leaf -> $d"
done

echo "sync-live-workspaces: OK"
