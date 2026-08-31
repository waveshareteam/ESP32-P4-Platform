#!/usr/bin/env python3
"""Discover ESP-IDF examples that should be built by CI."""

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


EXAMPLES_ROOT = Path("examples/esp-idf")
GLOBAL_EXAMPLE_PATTERNS = (
    ".github/workflows/esp-idf-examples.yml",
    ".github/scripts/discover_esp_idf_examples.py",
    ".github/scripts/ci_change_routing.py",
    ".github/scripts/tests/test_ci_discovery.py",
    "config/esp32p4_local_defaults.cmake",
    "config/esp32p4_rev1_3.defaults",
    "config/esp32p4_rev3_x.defaults",
)
ARDUINO_ONLY_PATTERNS = (
    ".github/workflows/arduino-examples.yml",
    ".github/scripts/discover_arduino_examples.py",
    ".github/scripts/package_arduino_firmware.py",
    ".github/scripts/tests/test_arduino_serial_readiness.py",
    ".github/scripts/tests/test_package_arduino_firmware.py",
)


def run_git(args: list[str]) -> list[str]:
    result = subprocess.run(
        ["git", *args],
        check=True,
        text=True,
        stdout=subprocess.PIPE,
    )
    return result.stdout.splitlines()


def list_examples() -> list[str]:
    if not EXAMPLES_ROOT.exists():
        return []

    examples = []
    for path in EXAMPLES_ROOT.iterdir():
        if (path / "CMakeLists.txt").is_file() and (path / "main").is_dir():
            examples.append(path.as_posix())
    return sorted(examples)


def normalize_example(value: str) -> str:
    value = value.strip().strip("/")
    if not value:
        return value
    if value == "all":
        return value
    if value.startswith(EXAMPLES_ROOT.as_posix() + "/"):
        return value
    return (EXAMPLES_ROOT / value).as_posix()


def route_changed_examples(
    *,
    known_examples: set[str],
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
        known_entries=known_examples,
        root=PurePosixPath(EXAMPLES_ROOT.as_posix()),
        other_framework_root=PurePosixPath("examples/arduino"),
        global_patterns=GLOBAL_EXAMPLE_PATTERNS,
        other_framework_patterns=ARDUINO_ONLY_PATTERNS,
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
    parser.add_argument("--example", default="")
    args = parser.parse_args()

    known_examples = set(list_examples())
    if not known_examples:
        print("No first-party ESP-IDF examples were discovered.", file=sys.stderr)
        return 2
    requested_example = normalize_example(args.example)
    if requested_example and (args.changed_files_from or args.base_ref):
        print(
            "--example cannot be combined with changed-file selection",
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

    if requested_example == "all":
        selected = sorted(known_examples)
    elif requested_example:
        if requested_example not in known_examples:
            print(f"Unknown ESP-IDF example: {args.example}", file=sys.stderr)
            print("Known examples:", file=sys.stderr)
            for example in sorted(known_examples):
                print(f"  {example}", file=sys.stderr)
            return 1
        selected = [requested_example]
    else:
        try:
            routing = route_changed_examples(
                known_examples=known_examples,
                changed_files_from=args.changed_files_from,
                base_ref=args.base_ref,
                head_ref=args.head_ref,
            )
        except (ChangeListError, subprocess.CalledProcessError) as exc:
            print(f"Could not classify changed files: {exc}", file=sys.stderr)
            return 2
        selected = list(routing.selected)

    matrix = {"example": selected}
    matrix_json = json.dumps(matrix, separators=(",", ":"))
    has_examples = "true" if selected else "false"

    github_output("matrix", matrix_json)
    github_output("has_examples", has_examples)
    github_output("examples", ",".join(selected))
    github_output("docs_only", "true" if routing.docs_only else "false")
    github_output("changed_path_count", str(routing.changed_path_count))
    github_output("firmware_paths", json.dumps(routing.firmware_paths))
    github_output("unknown_paths", json.dumps(routing.unknown_paths))
    github_output("removed_examples", json.dumps(routing.removed_entries))

    print(matrix_json)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
