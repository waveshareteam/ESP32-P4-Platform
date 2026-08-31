#!/usr/bin/env python3
"""Create a verified, profile-specific ESP32-P4 firmware delivery artifact.

The script intentionally consumes only an ESP-IDF build directory and an
explicit sdkconfig.  It rejects malformed build metadata instead of guessing
at a flash layout, so the produced split files are safe to use as the default
delivery form.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import stat
import sys
from dataclasses import dataclass
from pathlib import Path, PurePosixPath
from typing import Any


CAPACITY_BYTES = 32 * 1024 * 1024
PROFILES = ("rev1_3", "rev3_x")
EVENTS = ("pull_request", "push", "workflow_dispatch")
SHA_RE = re.compile(r"^[0-9a-fA-F]{40}$")
POSITIVE_DECIMAL_RE = re.compile(r"^[1-9][0-9]*$")
SAFE_NAME_RE = re.compile(r"[^A-Za-z0-9._-]+")


class PackageError(ValueError):
    """An input cannot safely produce a firmware artifact."""


@dataclass(frozen=True)
class FlashFile:
    offset: int
    source: Path
    artifact_path: str
    size: int
    sha256: str


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def read_json(path: Path, label: str) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise PackageError(f"invalid {label}: {exc}") from exc
    if not isinstance(value, dict):
        raise PackageError(f"{label} must be a JSON object")
    return value


def require_regular_file(path: Path, label: str) -> Path:
    try:
        file_stat = path.stat()
    except OSError as exc:
        raise PackageError(f"missing {label}: {path}") from exc
    if not stat.S_ISREG(file_stat.st_mode) or file_stat.st_size <= 0:
        raise PackageError(f"{label} must be a non-empty regular file: {path}")
    return path


def require_inside(path: Path, root: Path, label: str) -> Path:
    try:
        resolved = path.resolve(strict=True)
    except OSError as exc:
        raise PackageError(f"missing {label}: {path}") from exc
    try:
        resolved.relative_to(root)
    except ValueError as exc:
        raise PackageError(f"{label} escapes build directory: {path}") from exc
    return resolved


def source_path(build_dir: Path, value: object) -> Path:
    if not isinstance(value, str) or not value:
        raise PackageError("flash file path must be a non-empty string")
    if "\\" in value or ":" in value:
        raise PackageError(f"unsafe flash file path: {value!r}")
    pure = PurePosixPath(value)
    if pure.is_absolute() or any(part in ("", ".", "..") for part in pure.parts):
        raise PackageError(f"unsafe flash file path: {value!r}")
    candidate = build_dir.joinpath(*pure.parts)
    return require_regular_file(require_inside(candidate, build_dir, "flash file"), "flash file")


def parse_offset(value: object) -> int:
    if not isinstance(value, str) or not value:
        raise PackageError("flash offset must be a non-empty string")
    try:
        offset = int(value, 0)
    except ValueError as exc:
        raise PackageError(f"invalid flash offset: {value!r}") from exc
    if offset < 0:
        raise PackageError(f"flash offset must not be negative: {value!r}")
    return offset


def safe_artifact_name(offset: int, source: Path) -> str:
    name = SAFE_NAME_RE.sub("_", source.name).strip("._")
    if not name:
        raise PackageError(f"flash file has unsafe name: {source.name!r}")
    return f"bin/{offset:08x}_{name}"


def contains_c6_name(value: str) -> bool:
    lowered = value.casefold()
    return "esp32c6" in lowered or re.search(r"(?:^|[^a-z0-9])c6(?:[^a-z0-9]|$)", lowered) is not None


def parse_flash_files(build_dir: Path, flash_files: object) -> list[FlashFile]:
    if not isinstance(flash_files, dict) or not flash_files:
        raise PackageError("flasher_args.json flash_files must be a non-empty object")
    parsed: list[FlashFile] = []
    offsets: set[int] = set()
    for raw_offset, raw_path in flash_files.items():
        offset = parse_offset(raw_offset)
        if offset in offsets:
            raise PackageError(f"duplicate flash offset: {raw_offset}")
        offsets.add(offset)
        if isinstance(raw_path, str) and contains_c6_name(raw_path):
            raise PackageError(f"C6-named image is not permitted: {raw_path}")
        source = source_path(build_dir, raw_path)
        if contains_c6_name(source.name):
            raise PackageError(f"C6-named image is not permitted: {source.name}")
        size = source.stat().st_size
        if offset + size > CAPACITY_BYTES:
            raise PackageError(f"flash file exceeds 32 MiB capacity: {raw_offset} {raw_path}")
        parsed.append(FlashFile(offset, source, safe_artifact_name(offset, source), size, sha256_file(source)))
    parsed.sort(key=lambda item: item.offset)
    previous_end = 0
    for item in parsed:
        if item.offset < previous_end:
            raise PackageError(f"flash file overlaps a preceding segment: {item.artifact_path}")
        previous_end = item.offset + item.size
    return parsed


def parse_sdkconfig(path: Path, profile: str) -> tuple[dict[str, str], dict[str, object]]:
    require_regular_file(path, "sdkconfig")
    values: dict[str, str] = {}
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except (OSError, UnicodeDecodeError) as exc:
        raise PackageError(f"cannot read sdkconfig: {exc}") from exc
    for line in lines:
        if line.startswith("CONFIG_") and "=" in line:
            key, value = line.split("=", 1)
            values[key] = value
    required = {
        "CONFIG_ESPTOOLPY_FLASHSIZE_32MB": "y",
        "CONFIG_ESP32P4_SELECTS_REV_LESS_V3": "y" if profile == "rev1_3" else None,
        "CONFIG_ESP32P4_REV_MIN_100": "y" if profile == "rev1_3" else None,
        "CONFIG_ESP32P4_REV_MIN_300": "y" if profile == "rev3_x" else None,
    }
    if values.get("CONFIG_ESPTOOLPY_FLASHSIZE_32MB") != "y":
        raise PackageError("sdkconfig does not select 32 MiB flash")
    if profile == "rev1_3":
        if values.get("CONFIG_ESP32P4_SELECTS_REV_LESS_V3") != "y" or values.get("CONFIG_ESP32P4_REV_MIN_100") != "y":
            raise PackageError("sdkconfig does not satisfy rev1_3 revision contract")
        if values.get("CONFIG_ESP32P4_REV_MIN_300") == "y":
            raise PackageError("sdkconfig selects rev3_x minimum for rev1_3")
    else:
        if values.get("CONFIG_ESP32P4_SELECTS_REV_LESS_V3", "n") != "n" or values.get("CONFIG_ESP32P4_REV_MIN_300") != "y":
            raise PackageError("sdkconfig does not satisfy rev3_x revision contract")
        if values.get("CONFIG_ESP32P4_REV_MIN_100") == "y":
            raise PackageError("sdkconfig selects rev1_3 minimum for rev3_x")
    evidence_keys = tuple(required)
    contract = {
        "profile": profile,
        "selects_rev_less_v3": "y" if profile == "rev1_3" else "n_or_unset",
        "revision_minimum": "100" if profile == "rev1_3" else "300",
        "opposite_minimum_selected": False,
    }
    evidence = {key: values.get(key, "<unset>") for key in evidence_keys}
    return values, {"revision_contract": contract, "config_evidence": evidence}


def validate_settings(settings: object) -> dict[str, str]:
    if not isinstance(settings, dict):
        raise PackageError("flasher_args.json flash_settings must be an object")
    required = ("flash_mode", "flash_freq", "flash_size")
    result: dict[str, str] = {}
    for key in required:
        value = settings.get(key)
        if not isinstance(value, str) or not value or any(char.isspace() for char in value):
            raise PackageError(f"flasher_args.json flash_settings.{key} must be a non-empty token")
        result[key] = value
    if result["flash_size"].casefold() != "32mb":
        raise PackageError("flasher_args.json flash_settings.flash_size must be 32MB")
    return result


def identity(value: str, label: str) -> str:
    if not SHA_RE.fullmatch(value):
        raise PackageError(f"{label} must be a 40-character hexadecimal SHA")
    return value.lower()


def nonempty_token(value: str, label: str) -> str:
    if not value or any(char in value for char in "\r\n\x00"):
        raise PackageError(f"{label} must be non-empty and single-line")
    return value


def positive_decimal(value: str, label: str) -> str:
    if not POSITIVE_DECIMAL_RE.fullmatch(value):
        raise PackageError(f"{label} must be a positive decimal integer")
    return value


def flash_arguments(settings: dict[str, str], files: list[FlashFile]) -> list[str]:
    arguments = [
        "--chip", "esp32p4",
        "write_flash",
        "--flash_mode", settings["flash_mode"],
        "--flash_freq", settings["flash_freq"],
        "--flash_size", settings["flash_size"],
    ]
    for item in files:
        arguments.extend((f"0x{item.offset:x}", item.artifact_path))
    return arguments


def shell_quote(value: str) -> str:
    return "'" + value.replace("'", "'\"'\"'") + "'"


def write_merged(path: Path, files: list[FlashFile]) -> tuple[int, str]:
    fill = b"\xff" * (1024 * 1024)
    with path.open("wb") as merged:
        for _ in range(CAPACITY_BYTES // len(fill)):
            merged.write(fill)
    with path.open("r+b") as merged:
        for item in files:
            merged.seek(item.offset)
            with item.source.open("rb") as source:
                shutil.copyfileobj(source, merged, length=1024 * 1024)
    return path.stat().st_size, sha256_file(path)


def write_text(path: Path, content: str) -> None:
    path.write_text(content, encoding="utf-8", newline="\n")


def package(args: argparse.Namespace) -> Path:
    project = Path(args.project).resolve(strict=True)
    if not project.is_dir():
        raise PackageError("project must be a directory")
    build_dir = Path(args.build_dir).resolve(strict=True)
    if not build_dir.is_dir():
        raise PackageError("build-dir must be a directory")
    sdkconfig = Path(args.sdkconfig).resolve(strict=True)
    source_sha = identity(args.source_sha, "source-sha")
    pr_head_sha = identity(args.pr_head_sha, "pr-head-sha")
    if source_sha != pr_head_sha:
        raise PackageError("source-sha and pr-head-sha must identify the same revision")
    event = nonempty_token(args.event, "event")
    if event not in EVENTS:
        raise PackageError(f"event must be one of: {', '.join(EVENTS)}")
    run_id = positive_decimal(args.run_id, "run-id")
    run_attempt = positive_decimal(args.run_attempt, "run-attempt")
    idf_version = nonempty_token(args.idf_version, "idf-version")

    description = read_json(require_regular_file(build_dir / "project_description.json", "project_description.json"), "project_description.json")
    if description.get("target") != "esp32p4":
        raise PackageError("project_description.json target must be esp32p4")
    flasher = read_json(require_regular_file(build_dir / "flasher_args.json", "flasher_args.json"), "flasher_args.json")
    if "target" in flasher and flasher["target"] != "esp32p4":
        raise PackageError("flasher_args.json target must be esp32p4")
    settings = validate_settings(flasher.get("flash_settings"))
    files = parse_flash_files(build_dir, flasher.get("flash_files"))
    _, configuration = parse_sdkconfig(sdkconfig, args.profile)

    output = Path(args.output_dir)
    if output.exists():
        if output.is_symlink() or not output.is_dir() or any(output.iterdir()):
            raise PackageError("output-dir must be absent or an empty non-symlink directory")
    else:
        output.mkdir(parents=True)
    bin_dir = output / "bin"
    bin_dir.mkdir()
    for item in files:
        destination = output / item.artifact_path
        shutil.copyfile(item.source, destination)

    portable_flasher = {
        "flash_files": {f"0x{item.offset:x}": item.artifact_path for item in files},
        "flash_settings": settings,
        "target": "esp32p4",
    }
    write_text(output / "flasher_args.json", json.dumps(portable_flasher, indent=2, sort_keys=True) + "\n")
    command = flash_arguments(settings, files)
    write_text(output / "flash_args", " ".join(command) + "\n")
    write_text(output / "flash.sh", "#!/usr/bin/env sh\nset -eu\nesptool.py " + " ".join(shell_quote(value) for value in command) + "\n")
    write_text(output / "flash.bat", "@echo off\r\nesptool.py " + " ".join(command) + "\r\n")
    write_text(output / "sdkconfig.txt", sdkconfig.read_text(encoding="utf-8"))
    merged_size, merged_sha256 = write_merged(output / "merged.bin", files)

    sums = [(item.sha256, item.artifact_path) for item in files]
    sums.append((merged_sha256, "merged.bin"))
    write_text(output / "SHA256SUMS", "".join(f"{digest} *{name}\n" for digest, name in sums))
    manifest = {
        "schema_version": 2,
        "profile": args.profile,
        "target": "esp32p4",
        "source_sha": source_sha,
        "pr_head_sha": pr_head_sha,
        "event": event,
        "run_id": run_id,
        "run_attempt": run_attempt,
        "idf_version": idf_version,
        "capacity_bytes": CAPACITY_BYTES,
        "capacity": "32MiB",
        **configuration,
        "c6_binary_included": False,
        "erase_required": False,
        "flash_by_default": "split_files",
        "flash_files": [
            {"offset": f"0x{item.offset:x}", "path": item.artifact_path, "size": item.size, "sha256": item.sha256}
            for item in files
        ],
        "merged_image": {"path": "merged.bin", "size": merged_size, "sha256": merged_sha256, "flash_by_default": False},
        "retention_days": 14,
    }
    write_text(output / "manifest.json", json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    readme = """# ESP32-P4 product firmware\n\nThis package targets ESP32-P4 with a 32 MiB flash device. Use the split files\nby default: run `flash.sh` on POSIX hosts or `flash.bat` on Windows. Both\nhelpers use `write_flash` and write only the listed regions.\n\n`merged.bin` is a full 32 MiB image for tooling that explicitly requires one.\nVerify the listed hashes before use.\n"""
    write_text(output / "README.md", readme)
    return output


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", required=True)
    parser.add_argument("--build-dir", required=True)
    parser.add_argument("--sdkconfig", required=True)
    parser.add_argument("--profile", choices=PROFILES, required=True)
    parser.add_argument("--output-dir", required=True)
    parser.add_argument("--source-sha", required=True)
    parser.add_argument("--pr-head-sha", required=True)
    parser.add_argument("--event", required=True)
    parser.add_argument("--run-id", required=True)
    parser.add_argument("--run-attempt", required=True)
    parser.add_argument("--idf-version", required=True)
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        output = package(args)
    except PackageError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2
    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
