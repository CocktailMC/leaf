#!/usr/bin/env bash
# Headless live runServer smoke: start, wait for Done, then stop.
# Usage: ./scripts/live-runserver-smoke.sh fabric|neoforge
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LOADER="${1:-fabric}"
JDK="${LEAF_JDK:-$HOME/.local/jdk25}"
export JAVA_HOME="${JAVA_HOME:-$JDK}"

case "$LOADER" in
  fabric) DIR="$ROOT/bridge/fabric-live" ;;
  neoforge) DIR="$ROOT/bridge/neoforge-live" ;;
  *) echo "usage: $0 fabric|neoforge" >&2; exit 2 ;;
esac

"$ROOT/scripts/sync-live-workspaces.sh"

LOG="$(mktemp)"
cd "$DIR"
./gradlew runServer --quiet >"$LOG" 2>&1 &
PID=$!
cleanup() {
  kill "$PID" 2>/dev/null || true
  # Child JVM may outlive gradle wrapper.
  pkill -P "$PID" 2>/dev/null || true
  # Ensure the game port is released before the next loader smoke.
  fuser -k 25565/tcp >/dev/null 2>&1 || true
}
trap cleanup EXIT

# Avoid racing a leftover runServer from a previous smoke.
fuser -k 25565/tcp >/dev/null 2>&1 || true
sleep 1

ok=0
for _ in $(seq 1 240); do
  if rg -q 'Done \(' "$LOG"; then
    ok=1
    break
  fi
  if ! kill -0 "$PID" 2>/dev/null; then
    break
  fi
  sleep 1
done

if [[ "$ok" -ne 1 ]]; then
  echo "live-runserver-smoke ($LOADER): FAILED — see $LOG" >&2
  rg -n 'Exception|Error|SIGSEGV|UnsatisfiedLink|FAILED' "$LOG" | head -40 >&2 || true
  exit 1
fi

if ! rg -q 'greeter on_enable' "$LOG"; then
  echo "live-runserver-smoke ($LOADER): Done but greeter did not enable" >&2
  exit 1
fi

if [[ ! -d "$DIR/run/leafmods/greeter.leaf" ]] \
  && [[ ! -d "$DIR/run/server/leafmods/greeter.leaf" ]]; then
  echo "live-runserver-smoke ($LOADER): expected greeter.leaf in run leafmods" >&2
  exit 1
fi

echo "live-runserver-smoke ($LOADER): OK"
rm -f "$LOG"
