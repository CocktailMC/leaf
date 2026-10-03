#!/usr/bin/env python3
"""LEAFMC leaf-cli — pack / install / list .leafmod / .leaf packages."""

from __future__ import annotations

import argparse
import json
import shutil
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tooling" / "packager"))
from leaf_pack import host_target  # noqa: E402


def pack_dir(manifest: Path, native: Path, out: Path, target: str | None) -> int:
    argv = [
        "leaf_pack.py",
        "--manifest",
        str(manifest),
        "--native",
        str(native),
        "--out",
        str(out),
    ]
    if target:
        argv += ["--target", target]
    old = sys.argv
    try:
        sys.argv = argv
        from leaf_pack import main as pack_main

        return pack_main()
    finally:
        sys.argv = old


def zip_dir(src_dir: Path, archive: Path) -> None:
    if archive.exists():
        archive.unlink()
    with zipfile.ZipFile(archive, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        for path in src_dir.rglob("*"):
            if path.is_file():
                zf.write(path, path.relative_to(src_dir).as_posix())


def cmd_pack(args: argparse.Namespace) -> int:
    out = Path(args.out)
    if args.zip:
        staging = out.parent / (out.stem + ".staging.leafmod")
        if staging.exists():
            shutil.rmtree(staging)
        rc = pack_dir(args.manifest, args.native, staging, args.target)
        if rc != 0:
            return rc
        zip_dir(staging, out)
        shutil.rmtree(staging)
        print(f"zipped -> {out}")
        return 0
    return pack_dir(args.manifest, args.native, out, args.target)


def is_leaf_package_name(name: str) -> bool:
    return name.endswith(".leafmod") or (
        name.endswith(".leaf") and not name.endswith(".leafmod")
    )


def cmd_install(args: argparse.Namespace) -> int:
    src = Path(args.package)
    dest_root = Path(args.leafmods)
    dest_root.mkdir(parents=True, exist_ok=True)
    if src.is_dir() and is_leaf_package_name(src.name):
        dest = dest_root / src.name
        if dest.exists():
            shutil.rmtree(dest)
        shutil.copytree(src, dest)
        print(f"installed directory package -> {dest}")
        return 0
    if src.is_file() and is_leaf_package_name(src.name):
        dest = dest_root / src.name
        if dest.exists():
            dest.unlink()
        shutil.copy2(src, dest)
        print(f"installed zip package -> {dest}")
        return 0
    print(f"not a .leafmod/.leaf package: {src}", file=sys.stderr)
    return 1


def cmd_list(args: argparse.Namespace) -> int:
    root = Path(args.leafmods)
    if not root.is_dir():
        print(f"missing leafmods dir: {root}", file=sys.stderr)
        return 1
    found = False
    for entry in sorted(root.iterdir()):
        if entry.name.startswith("."):
            continue
        if entry.is_dir() and is_leaf_package_name(entry.name):
            manifest = entry / "leaf.mod.json"
            mid = entry.name
            if manifest.is_file():
                mid = json.loads(manifest.read_text(encoding="utf-8")).get("id", mid)
            print(f"{mid}\tdir\t{entry}")
            found = True
        elif entry.is_file() and is_leaf_package_name(entry.name):
            mid = entry.stem
            try:
                with zipfile.ZipFile(entry) as zf:
                    with zf.open("leaf.mod.json") as fh:
                        mid = json.loads(fh.read().decode("utf-8")).get("id", mid)
            except Exception:
                pass
            print(f"{mid}\tzip\t{entry}")
            found = True
    if not found:
        print("(no leafmods)")
    return 0


def _read_manifest_from_package(path: Path) -> tuple[dict | None, str | None]:
    """Return (manifest_dict, error)."""
    if path.is_dir():
        manifest = path / "leaf.mod.json"
        if not manifest.is_file():
            return None, f"missing leaf.mod.json in {path}"
        try:
            return json.loads(manifest.read_text(encoding="utf-8")), None
        except (OSError, json.JSONDecodeError) as exc:
            return None, f"invalid leaf.mod.json: {exc}"
    if path.is_file():
        try:
            with zipfile.ZipFile(path) as zf:
                with zf.open("leaf.mod.json") as fh:
                    return json.loads(fh.read().decode("utf-8")), None
        except KeyError:
            return None, f"zip missing leaf.mod.json: {path}"
        except (OSError, zipfile.BadZipFile, json.JSONDecodeError) as exc:
            return None, f"invalid zip package: {exc}"
    return None, f"not found: {path}"


def _package_has_native(path: Path, target: str) -> bool:
    rel = f"native/{target}/"
    if path.is_dir():
        native_dir = path / "native" / target
        if not native_dir.is_dir():
            return False
        return any(native_dir.iterdir())
    if path.is_file():
        try:
            with zipfile.ZipFile(path) as zf:
                return any(
                    name.startswith(rel) and not name.endswith("/")
                    for name in zf.namelist()
                )
        except (OSError, zipfile.BadZipFile):
            return False
    return False


def cmd_verify(args: argparse.Namespace) -> int:
    """Validate a .leaf / .leafmod package is loadable."""
    path = Path(args.package)
    if not is_leaf_package_name(path.name):
        print(f"FAIL name must end with .leaf or .leafmod: {path.name}", file=sys.stderr)
        return 1
    manifest, err = _read_manifest_from_package(path)
    if err:
        print(f"FAIL {err}", file=sys.stderr)
        return 1
    assert manifest is not None
    ok = True
    for key in ("id", "name", "version", "entry"):
        if not manifest.get(key):
            print(f"FAIL manifest missing '{key}'")
            ok = False
        else:
            print(f"OK   manifest.{key}={manifest[key]}")
    leaf = manifest.get("leaf") if isinstance(manifest.get("leaf"), dict) else {}
    api = leaf.get("api", manifest.get("leaf_api"))
    if api is None:
        print("FAIL manifest missing leaf.api (or leaf_api)")
        ok = False
    else:
        print(f"OK   manifest.leaf.api={api}")
    if not manifest.get("runtime"):
        print("WARN manifest missing 'runtime' (expected native)")
    else:
        print(f"OK   manifest.runtime={manifest['runtime']}")
    target = args.target or host_target()
    if _package_has_native(path, target):
        print(f"OK   native/{target}/ present")
    else:
        print(f"FAIL missing native/{target}/ for this host")
        ok = False
    kind = "dir" if path.is_dir() else "zip"
    print(f"{'verify: OK' if ok else 'verify: FAILED'} ({kind} {path})")
    return 0 if ok else 1


def cmd_host(_args: argparse.Namespace) -> int:
    print(host_target())
    return 0


REQUIRED_BRIDGE_SYMBOLS = (
    "leaf_bridge_init_flat",
    "leaf_bridge_set_send_player_message_hook",
    "leaf_bridge_set_broadcast_message_hook",
    "leaf_bridge_set_inventory_hooks",
    "leaf_bridge_set_inventory_stack_hooks",
    "leaf_bridge_set_get_block_hook",
    "leaf_bridge_set_world_hooks",
    "leaf_bridge_set_player_pos_hooks",
    "leaf_bridge_set_player_health_hooks",
    "leaf_bridge_set_player_food_hooks",
    "leaf_bridge_set_player_gamemode_hooks",
    "leaf_bridge_set_player_xp_hooks",
    "leaf_bridge_set_player_look_hooks",
    "leaf_bridge_set_play_sound_hook",
    "leaf_bridge_set_actionbar_hook",
    "leaf_bridge_set_title_hook",
    "leaf_bridge_set_kick_player_hook",
    "leaf_bridge_set_give_item_hook",
    "leaf_bridge_set_effect_hooks",
    "leaf_bridge_set_spawn_particle_hook",
    "leaf_bridge_set_world_time_hooks",
    "leaf_bridge_set_player_velocity_hooks",
    "leaf_bridge_set_player_flags_hook",
    "leaf_bridge_set_run_command_hook",
    "leaf_bridge_set_clear_inventory_hook",
    "leaf_bridge_set_player_flight_hook",
    "leaf_bridge_set_get_biome_hook",
    "leaf_bridge_set_difficulty_hooks",
    "leaf_bridge_set_weather_hooks",
    "leaf_bridge_set_get_light_level_hook",
    "leaf_bridge_set_player_latency_hook",
    "leaf_bridge_set_world_spawn_hooks",
    "leaf_bridge_set_player_op_hook",
    "leaf_bridge_set_player_uuid_hook",
    "leaf_bridge_set_player_permission_hook",
    "leaf_bridge_set_find_player_uuid_hook",
    "leaf_bridge_set_find_player_name_hook",
    "leaf_bridge_set_block_registry_hooks",
    "leaf_bridge_set_give_item_registry_hook",
    "leaf_bridge_set_inventory_registry_hooks",
    "leaf_bridge_set_teleport_hook",
    "leaf_bridge_set_selected_slot_hooks",
    "leaf_bridge_set_get_world_seed_hook",
    "leaf_bridge_set_player_absorption_hooks",
    "leaf_bridge_set_player_invulnerable_hooks",
    "leaf_bridge_set_player_air_hooks",
    "leaf_bridge_set_player_fire_ticks_hooks",
    "leaf_bridge_set_player_frozen_ticks_hooks",
    "leaf_bridge_set_player_no_gravity_hooks",
    "leaf_bridge_set_player_silent_hooks",
    "leaf_bridge_set_player_glowing_hooks",
    "leaf_bridge_set_player_invisible_hooks",
    "leaf_bridge_set_player_portal_cooldown_hooks",
    "leaf_bridge_set_get_player_max_air_hook",
    "leaf_bridge_register_player",
    "leaf_bridge_unregister_player",
    "leaf_bridge_emit_player_chat",
    "leaf_bridge_emit_player_death",
    "leaf_bridge_emit_entity_spawn",
    "leaf_bridge_emit_entity_remove",
    "leaf_bridge_emit_world_load",
    "leaf_bridge_emit_block_break",
    "leaf_bridge_emit_block_place",
)


def cmd_doctor(_args: argparse.Namespace) -> int:
    """Check native host + greeter packaging look usable."""
    import subprocess

    native = ROOT / "build" / "engine" / "bridge-host" / "libleaf_bridge.so"
    greeter = ROOT / "dist" / "leafmods" / "greeter.leafmod"
    ok = True
    if not native.is_file():
        print(f"FAIL missing {native}")
        ok = False
    else:
        print(f"OK   native {native}")
        try:
            out = subprocess.check_output(["nm", "-D", str(native)], text=True)
        except (OSError, subprocess.CalledProcessError) as exc:
            print(f"FAIL nm: {exc}")
            ok = False
            out = ""
        for sym in REQUIRED_BRIDGE_SYMBOLS:
            if f" {sym}" in out or out.rstrip().endswith(sym):
                print(f"OK   symbol {sym}")
            else:
                # nm format: "T leaf_bridge_..."
                if any(line.split()[-1] == sym for line in out.splitlines() if line.strip()):
                    print(f"OK   symbol {sym}")
                else:
                    print(f"FAIL missing symbol {sym}")
                    ok = False

    if greeter.is_dir() or greeter.is_file():
        print(f"OK   greeter {greeter}")
    else:
        print(f"FAIL missing {greeter} (run ./scripts/smoke-three-loaders.sh)")
        ok = False

    print("doctor: OK" if ok else "doctor: FAILED")
    return 0 if ok else 1


def cmd_sync_live(_args: argparse.Namespace) -> int:
    script = ROOT / "scripts" / "sync-live-workspaces.sh"
    import subprocess

    return subprocess.call([str(script)])


def main() -> int:
    ap = argparse.ArgumentParser(prog="leaf", description="LEAFMC Leaf Mod tooling")
    sub = ap.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser(
        "pack", help="Package a .leafmod/.leaf directory (or zip with --zip)"
    )
    p.add_argument("--manifest", required=True, type=Path)
    p.add_argument("--native", required=True, type=Path)
    p.add_argument("--out", required=True, type=Path)
    p.add_argument("--target", default=None)
    p.add_argument(
        "--zip", action="store_true", help="Emit a zip .leafmod/.leaf archive"
    )
    p.set_defaults(func=cmd_pack)

    p = sub.add_parser(
        "install", help="Install a .leafmod/.leaf package into a leafmods directory"
    )
    p.add_argument("package", type=Path)
    p.add_argument("--leafmods", type=Path, default=Path("leafmods"))
    p.set_defaults(func=cmd_install)

    p = sub.add_parser(
        "list", help="List .leafmod/.leaf packages in a leafmods directory"
    )
    p.add_argument("--leafmods", type=Path, default=Path("leafmods"))
    p.set_defaults(func=cmd_list)

    p = sub.add_parser(
        "verify", help="Validate a .leafmod/.leaf package (manifest + native)"
    )
    p.add_argument("package", type=Path)
    p.add_argument(
        "--target",
        default=None,
        help="native target triple (default: host)",
    )
    p.set_defaults(func=cmd_verify)

    p = sub.add_parser("host", help="Print detected host target triple")
    p.set_defaults(func=cmd_host)

    p = sub.add_parser("doctor", help="Verify native host symbols + greeter package")
    p.set_defaults(func=cmd_doctor)

    p = sub.add_parser("sync-live", help="Sync native + greeter into live run dirs")
    p.set_defaults(func=cmd_sync_live)

    args = ap.parse_args()
    return int(args.func(args))


if __name__ == "__main__":
    raise SystemExit(main())
