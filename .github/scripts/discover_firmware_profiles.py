#!/usr/bin/env python3
"""Route product-firmware changes to supported ESP32-P4 revision profiles."""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path

from ci_change_routing import Change, ChangeListError, is_documentation_path, read_name_status


PROFILES = ("rev1_3", "rev3_x")
FIRMWARE_ROOT = "firmware/"
FACTORY_ROOT = "firmware/factory_firmware/"
PRODUCT_BUILD_INPUTS = {
    ".github/workflows/product-firmware.yml",
    ".github/scripts/package_product_firmware.py",
}
KNOWN_NON_PRODUCT_PATHS = {
    ".gitignore",
    "Flash-CI-Firmware.cmd",
    "config/esp32p4_local_defaults.cmake",
    "config/esp32p4_rev1_3.defaults",
    "config/esp32p4_rev3_x.defaults",
    "config/esp32p4_rev_pre_v3.defaults",
    "config/esp32p4_rev_v3_0.defaults",
    "config/esp32p4_rev_v3_1.defaults",
    "config/markdown-audit.json",
    "scripts/Flash-CI-Firmware.ps1",
}
KNOWN_NON_PRODUCT_PREFIXES = (
    ".github/",
    "docs/",
    "examples/",
)


def selected_profiles(profile: str) -> tuple[str, ...]:
    """Expand a requested profile selector."""

    return PROFILES if profile == "all" else (profile,)


def requires_product_build(path: str) -> bool:
    """Return whether an evidenced product input affects both firmware profiles."""

    if path in PRODUCT_BUILD_INPUTS:
        return True
    if path.startswith("firmware/brookesia/"):
        return not is_documentation_path(path)
    return False


def is_known_non_product_path(path: str) -> bool:
    """Return whether a path is evidenced as outside product-firmware builds."""

    if path in KNOWN_NON_PRODUCT_PATHS:
        return True
    return path.startswith(KNOWN_NON_PRODUCT_PREFIXES)


def route_changes(changes: list[Change], requested_profile: str) -> dict[str, object]:
    """Classify a complete name-status change list without building firmware."""

    if not changes:
        raise ChangeListError("changed-file evidence is empty")

    paths = {change.path for change in changes}
    firmware_paths = sorted(path for path in paths if path.startswith(FIRMWARE_ROOT))
    release_paths = sorted(path for path in paths if path.startswith(FACTORY_ROOT))
    unknown_paths: list[str] = []
    build_required = False

    for path in sorted(paths):
        if path.startswith(FACTORY_ROOT):
            continue
        if is_documentation_path(path):
            continue
        if requires_product_build(path):
            build_required = True
            continue
        if is_known_non_product_path(path):
            continue
        unknown_paths.append(path)

    allowed = set(selected_profiles(requested_profile))
    profiles = tuple(
        profile
        for profile in PROFILES
        if build_required and not unknown_paths and profile in allowed
    )
    return {
        "matrix": {"profile": list(profiles)},
        "has_profiles": bool(profiles),
        "profiles": list(profiles),
        "docs_only": all(is_documentation_path(path) for path in paths),
        "changed_path_count": len(paths),
        "firmware_paths": firmware_paths,
        "release_paths": release_paths,
        "unknown_paths": unknown_paths,
    }


def read_diff_refs(base_ref: str, head_ref: str) -> list[Change]:
    """Read a Git name-status diff and turn failures into a closed selector error."""

    result = subprocess.run(
        ["git", "diff", "--name-status", "--find-renames", f"{base_ref}...{head_ref}"],
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if result.returncode:
        message = result.stderr.strip() or "git diff failed"
        raise ChangeListError(message)
    return parse_name_status_text(result.stdout)


def parse_name_status_text(text: str) -> list[Change]:
    """Parse text diff output while retaining both sides of a rename or deletion."""

    # Import through the shared parser so this selector has the same Git semantics
    # as the existing example discovery workflows.
    from ci_change_routing import parse_name_status

    return parse_name_status(text.splitlines())


def write_github_output(result: dict[str, object]) -> None:
    """Expose the workflow contract through GITHUB_OUTPUT when available."""

    output_path = os.environ.get("GITHUB_OUTPUT")
    if not output_path:
        return
    values = {
        "matrix": json.dumps(result["matrix"], separators=(",", ":")),
        "has_profiles": str(result["has_profiles"]).lower(),
        "profiles": ",".join(result["profiles"]),
        "docs_only": str(result["docs_only"]).lower(),
        "changed_path_count": str(result["changed_path_count"]),
        "firmware_paths": json.dumps(result["firmware_paths"], separators=(",", ":")),
        "release_paths": json.dumps(result["release_paths"], separators=(",", ":")),
        "unknown_paths": json.dumps(result["unknown_paths"], separators=(",", ":")),
    }
    with Path(output_path).open("a", encoding="utf-8") as output:
        for key, value in values.items():
            output.write(f"{key}={value}\n")


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--profile", choices=("all", *PROFILES), default="all")
    source = parser.add_mutually_exclusive_group()
    source.add_argument("--changed-files-from", type=Path)
    source.add_argument("--base-ref")
    parser.add_argument("--head-ref")
    args = parser.parse_args(argv)
    if bool(args.base_ref) != bool(args.head_ref):
        parser.error("--base-ref and --head-ref must be supplied together")
    return args


def main(argv: list[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        if args.changed_files_from:
            result = route_changes(read_name_status(args.changed_files_from), args.profile)
        elif args.base_ref:
            result = route_changes(read_diff_refs(args.base_ref, args.head_ref), args.profile)
        else:
            profiles = selected_profiles(args.profile)
            result = {
                "matrix": {"profile": list(profiles)},
                "has_profiles": bool(profiles),
                "profiles": list(profiles),
                "docs_only": False,
                "changed_path_count": 0,
                "firmware_paths": [],
                "release_paths": [],
                "unknown_paths": [],
            }
    except ChangeListError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2

    write_github_output(result)
    print(json.dumps(result["matrix"], separators=(",", ":")))
    if result["unknown_paths"]:
        print(
            "error: unclassified non-document paths are outside the "
            "product-firmware routing policy:",
            file=sys.stderr,
        )
        for path in result["unknown_paths"]:
            print(f"  {path}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
