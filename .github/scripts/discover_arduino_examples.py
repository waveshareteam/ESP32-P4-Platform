#!/usr/bin/env python3
"""Discover first-party Arduino sketches that should be compiled by CI."""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path, PurePosixPath

from ci_change_routing import (
    ChangeListError,
    RoutingResult,
    parse_name_status,
    read_name_status,
    route_changes,
)


SKETCHES_ROOT = PurePosixPath("examples/arduino/examples")
GLOBAL_SKETCH_PATTERNS = (
    ".github/workflows/arduino-examples.yml",
    ".github/scripts/discover_arduino_examples.py",
    ".github/scripts/package_arduino_firmware.py",
    ".github/scripts/ci_change_routing.py",
    ".github/scripts/tests/test_arduino_serial_readiness.py",
    ".github/scripts/tests/test_ci_discovery.py",
    ".github/scripts/tests/test_package_arduino_firmware.py",
    "examples/arduino/common/**",
    "examples/arduino/libraries/**",
)
ESP_IDF_ONLY_PATTERNS = (
    ".github/workflows/esp-idf-examples.yml",
    ".github/scripts/discover_esp_idf_examples.py",
    "config/esp32p4_local_defaults.cmake",
    "config/esp32p4_rev1_3.defaults",
    "config/esp32p4_rev3_x.defaults",
)


def run_git(args: list[str]) -> list[str]:
    result = subprocess.run(
        ["git", *args],
        check=True,
        text=True,
        stdout=subprocess.PIPE,
    )
    return result.stdout.splitlines()


def normalize_path(value: str) -> str:
    return value.replace("\\", "/").strip().strip("/")


def list_sketches() -> list[str]:
    root = Path(SKETCHES_ROOT.as_posix())
    if not root.exists():
        return []

    sketches = []
    for path in root.iterdir():
        if path.is_dir() and (path / f"{path.name}.ino").is_file():
            sketches.append(path.as_posix())
    return sorted(sketches)


def normalize_sketch(value: str) -> str:
    value = normalize_path(value)
    if not value or value == "all":
        return value

    path = PurePosixPath(value)
    if path.suffix.lower() == ".ino":
        path = path.parent

    root = SKETCHES_ROOT.as_posix()
    normalized = path.as_posix()
    if normalized.startswith(root + "/"):
        return normalized
    return (SKETCHES_ROOT / normalized).as_posix()


def route_changed_sketches(
    *,
    known_sketches: set[str],
    changed_files_from: Path | None = None,
    base_ref: str | None = None,
    head_ref: str = "HEAD",
) -> RoutingResult:
    if changed_files_from is not None:
        changes = read_name_status(changed_files_from)
    elif base_ref:
        changes = parse_name_status(
            run_git(
                [
                    "diff",
                    "--name-status",
                    "--find-renames",
                    f"{base_ref}...{head_ref}",
                ]
            )
        )
    else:
        raise ChangeListError("provide --changed-files-from or --base-ref")

    return route_changes(
        changes,
        known_entries=known_sketches,
        root=SKETCHES_ROOT,
        other_framework_root=PurePosixPath("examples/esp-idf"),
        global_patterns=GLOBAL_SKETCH_PATTERNS,
        other_framework_patterns=ESP_IDF_ONLY_PATTERNS,
    )


def github_output(name: str, value: str) -> None:
    output_path = os.environ.get("GITHUB_OUTPUT")
    if output_path:
        with open(output_path, "a", encoding="utf-8") as output:
            output.write(f"{name}={value}\n")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--base-ref")
    parser.add_argument("--head-ref", default="HEAD")
    parser.add_argument("--changed-files-from", type=Path)
    parser.add_argument("--sketch", default="")
    args = parser.parse_args()

    known_sketches = set(list_sketches())
    if not known_sketches:
        print("No first-party Arduino sketches were discovered.", file=sys.stderr)
        return 2
    requested_sketch = normalize_sketch(args.sketch)
    if requested_sketch and (args.changed_files_from or args.base_ref):
        print(
            "--sketch cannot be combined with changed-file selection",
            file=sys.stderr,
        )
        return 2
    if args.changed_files_from and args.base_ref:
        print(
            "--changed-files-from and --base-ref are mutually exclusive",
            file=sys.stderr,
        )
        return 2

    routing = RoutingResult((), False, (), (), (), 0)

    if requested_sketch == "all":
        selected = sorted(known_sketches)
    elif requested_sketch:
        if requested_sketch not in known_sketches:
            print(f"Unknown Arduino sketch: {args.sketch}", file=sys.stderr)
            print("Known sketches:", file=sys.stderr)
            for sketch in sorted(known_sketches):
                print(f"  {sketch}", file=sys.stderr)
            return 1
        selected = [requested_sketch]
    else:
        try:
            routing = route_changed_sketches(
                known_sketches=known_sketches,
                changed_files_from=args.changed_files_from,
                base_ref=args.base_ref,
                head_ref=args.head_ref,
            )
        except (ChangeListError, subprocess.CalledProcessError) as exc:
            print(f"Could not classify changed files: {exc}", file=sys.stderr)
            return 2
        selected = list(routing.selected)

    matrix = {"sketch": selected}
    matrix_json = json.dumps(matrix, separators=(",", ":"))
    has_sketches = "true" if selected else "false"

    github_output("matrix", matrix_json)
    github_output("has_sketches", has_sketches)
    github_output("sketches", ",".join(selected))
    github_output("docs_only", "true" if routing.docs_only else "false")
    github_output("changed_path_count", str(routing.changed_path_count))
    github_output("firmware_paths", json.dumps(routing.firmware_paths))
    github_output("unknown_paths", json.dumps(routing.unknown_paths))
    github_output("removed_sketches", json.dumps(routing.removed_entries))

    print(matrix_json)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
