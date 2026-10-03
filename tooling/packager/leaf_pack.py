#!/usr/bin/env python3
"""Package a Leaf Mod directory into a *.leafmod folder layout.

Usage:
  leaf_pack.py --manifest leaf.mod.json --native libgreeter.so --out dist/greeter.leafmod
"""

from __future__ import annotations

import argparse
import json
import platform
import shutil
import sys
from pathlib import Path


def host_target() -> str:
    system = platform.system().lower()
    machine = platform.machine().lower()
    if system.startswith("linux"):
        os_part = "linux"
    elif system.startswith("darwin"):
        os_part = "macos"
    elif system.startswith("windows") or system.startswith("cygwin"):
        os_part = "windows"
    else:
        os_part = system

    if machine in ("x86_64", "amd64"):
        arch = "x86_64"
    elif machine in ("aarch64", "arm64"):
        arch = "aarch64" if os_part == "linux" else "arm64"
    else:
        arch = machine
    return f"{os_part}-{arch}"


def main() -> int:
    ap = argparse.ArgumentParser(description="Package a .leafmod directory")
    ap.add_argument("--manifest", required=True, type=Path)
    ap.add_argument("--native", required=True, type=Path, help="Shared library file")
    ap.add_argument("--out", required=True, type=Path, help="Output *.leafmod directory")
    ap.add_argument("--target", default=host_target())
    ap.add_argument("--lib-name", default=None, help="Filename under native/<target>/")
    args = ap.parse_args()

    if not args.manifest.is_file():
        print(f"manifest not found: {args.manifest}", file=sys.stderr)
        return 1
    if not args.native.is_file():
        print(f"native library not found: {args.native}", file=sys.stderr)
        return 1

    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    mod_id = manifest.get("id")
    if not mod_id:
        print("manifest missing id", file=sys.stderr)
        return 1

    out: Path = args.out
    if out.exists():
        shutil.rmtree(out)
    native_dir = out / "native" / args.target
    native_dir.mkdir(parents=True)

    shutil.copy2(args.manifest, out / "leaf.mod.json")
    lib_name = args.lib_name or args.native.name
    shutil.copy2(args.native, native_dir / lib_name)

    print(f"packed {mod_id} -> {out}")
    print(f"  native/{args.target}/{lib_name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
