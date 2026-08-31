#!/usr/bin/env python3
"""Build a safe, segmented flash archive from Arduino-ESP32 build output."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shlex
import stat
import struct
import subprocess
import tempfile
import zipfile
import zlib
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath


GIT_SHA_RE = re.compile(r"^[0-9a-f]{40}$")
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")
SAFE_VALUE_RE = re.compile(r"^[A-Za-z0-9._+-]+$")
SAFE_COMPONENT_RE = re.compile(r"^[A-Za-z0-9._-]+$")
TARGET_RE = re.compile(r"^[a-z0-9_-]+$")
FQBN_RE = re.compile(
    r"^[A-Za-z0-9._-]+:[A-Za-z0-9._-]+:[A-Za-z0-9._-]+$"
)
FLASH_SIZE_RE = re.compile(r"^(0[xX][0-9a-fA-F]+|[0-9]+)([KMG]i?B)?$")
SDKCONFIG_TARGET_RE = re.compile(r'^CONFIG_IDF_TARGET="([a-z0-9_-]+)"$', re.MULTILINE)
CHECKSUM_LINE_RE = re.compile(r"^([0-9a-f]{64})  (.+)$")
LINE_DIRECTIVE_RE = re.compile(r'^#line ([1-9][0-9]*) "([^"\r\n]+)"$')

PACKAGER_REPOSITORY_PATH = PurePosixPath(
    ".github/scripts/package_arduino_firmware.py"
)
SCRIPT_PATH = Path(__file__).absolute()

FLASH_SETTING_FLAGS = ("--flash-mode", "--flash-freq", "--flash-size")
REQUIRED_ARCHIVE_MEMBERS = {
    "manifest.json",
    "flash_args",
    "flash.sh",
    "flash.cmd",
    "metadata/build_identity.json",
    "metadata/flash_args.source.txt",
    "requirements.txt",
    "SHA256SUMS.txt",
}
CHECKSUMS_SELF_PATH = "SHA256SUMS.txt"
EXPECTED_ESPTOOL_VERSION = "5.3.1"
EXPECTED_ARTIFACT_TYPE = "source-built-arduino-segmented-flash"
EXPECTED_BSP_EVIDENCE = {
    "component": "waveshare/esp32_p4_platform",
    "version": "2.0.1",
    "component_hash": "b1be590dd7170ea417a3b950b81c7892ff266d6514070ecdd568faeb654b0149",
    "repository_commit": "bfeb6e6d5737178cdde78b630c8118074da0a657",
}
RESOLVED_PROPERTY_KEYS = (
    "_id",
    "version",
    "build.mcu",
    "build.chip_variant",
    "build.flash_mode",
    "build.flash_freq",
    "build.flash_size",
    "build.partitions",
    "build.usb_mode",
    "build.cdc_on_boot",
    "build.msc_on_boot",
    "build.dfu_on_boot",
    "upload.speed",
    "upload.use_1200bps_touch",
    "upload.wait_for_upload_port",
)
RAW_EVIDENCE_KEYS = (
    "build_options",
    "board_properties",
    "compile_commands",
    "primary_source",
    "generated_translation_unit",
    "sketch_object",
    "application_elf",
    "link_map",
    "compile_log",
)
MANIFEST_KEYS = {
    "schema_version",
    "artifact_type",
    "name",
    "project_path",
    "product_git_sha",
    "timestamp_utc",
    "framework",
    "build",
    "toolchain",
    "bsp",
    "flash",
    "integrity",
}
FRAMEWORK_KEYS = {"name", "version", "arduino_cli_version"}
BUILD_KEYS = {
    "fqbn",
    "resolved_fqbn",
    "board_options",
    "target",
    "sketch_name",
    "primary_source_basename",
    "generated_translation_unit_basename",
    "application_binary_basename",
    "identity_metadata_path",
    "build_options_sha256",
    "board_properties_sha256",
    "compile_commands_sha256",
    "primary_source_sha256",
    "generated_translation_unit_sha256",
    "source_line_count",
    "source_line_mapping_sha256",
    "compile_identity_sha256",
    "sketch_object_sha256",
    "application_elf_sha256",
    "link_map_sha256",
    "compile_log_sha256",
    "resolved_properties",
    "raw_evidence",
}
FLASH_KEYS = {
    "baud",
    "capacity_bytes",
    "segmented_payload_total",
    "settings",
    "offset_provenance",
    "recipe_provenance",
    "write_flash_command",
    "segments",
}
PUBLIC_TEXT_MEMBERS = {
    "manifest.json",
    "flash_args",
    "flash.sh",
    "flash.cmd",
    "metadata/build_identity.json",
    "metadata/flash_args.source.txt",
    "requirements.txt",
    "SHA256SUMS.txt",
}
PRIVATE_TEXT_PATTERNS = (
    re.compile(
        r"/(?:home|Users|tmp|private/tmp|var/folders|workspace|workdir)(?:/|\b)",
        re.IGNORECASE,
    ),
    re.compile(r"(?<![A-Za-z0-9+.-])[A-Za-z]:[\\\\/]"),
    re.compile(r"\\\\\\\\[^\\\\\s]+[\\\\][^\\\\\s]+"),
    re.compile(
        r"(?:^|[/\\\\\s])(?:\.cache|arduino15|cache|_work)(?:[/\\\\]|$)",
        re.IGNORECASE,
    ),
    re.compile(
        r"(?<![A-Za-z0-9._:+/\\-])/(?!/)(?:[A-Za-z0-9._-]+/)+[A-Za-z0-9._-]+"
    ),
)
PRIVATE_USER_TOKENS = tuple(
    sorted(
        {
            "ubuntu",
            "runner",
            "runneradmin",
            *(
                value
                for key in ("USER", "USERNAME", "LOGNAME")
                if (value := os.environ.get(key)) and len(value) >= 3
            ),
        },
        key=str.casefold,
    )
)
PRIVATE_BINARY_PATTERNS = (
    re.compile(
        rb"/(?:home|Users|tmp|private/tmp|var/folders|workspace|workdir)(?:/|\\\\)",
        re.IGNORECASE,
    ),
    re.compile(rb"[A-Za-z]:[\\\\/](?:Users|Documents and Settings)[\\\\/]", re.IGNORECASE),
    re.compile(rb"\\\\\\\\[^\\\\\x00\s]+\\\\[^\\\\\x00\s]+"),
    re.compile(
        rb"(?:^|[\\\\/])(?:\.cache|\.arduino15|arduino15|_work)(?:[\\\\/]|$)",
        re.IGNORECASE,
    ),
)
# Exact full printable runs observed in the baseline Arduino-ESP32
# 3.3.11 application binaries.  This is deliberately not a prefix or
# length-based exception for absolute paths.
OBSERVED_RANDOM_BINARY_PATH_RUNS = {
    b"  /l",
    b" /Ag",
    b" /cV",
    b" /qF",
    b"$ /0",
    b"/0o0",
    b"/540",
    b"/LPHL",
    b"/QG3",
    b"/XMc",
    b" /uU",
    b"@/cN",
    b"@/qF",
    b"\x60/cN",
    b"P:\\Uc",
}
STOCK_EXACT_BINARY_PATH_RUNS = {b"/dev/%s"}
PRINTABLE_RUN_RE = re.compile(rb"[ -~]{4,}")
# Paths below are emitted by Espressif's stock precompiled SDK/runtime.  This is
# deliberately an exact, narrow exception: a generic host path never inherits it.
ABSOLUTE_PATH_CANDIDATE_RE = re.compile(
    rb"(?:/{1,2}[A-Za-z0-9._@+\-]+(?:/[A-Za-z0-9._@+\-]+)*|[A-Za-z]:[\\/][^\x00\r\n ]+|\\\\[^\\\x00\r\n ]+\\[^\\\x00\r\n ]+)"
)
COMPILER_ABSOLUTE_PATH_PREFIX_RE = re.compile(
    rb"(?:^|[\s\"'])(?:-I|-L|-isystem|-iquote|-idirafter|-include|-imacros|"
    rb"--sysroot(?:=)?)(?:=)?$"
)
PUBLIC_NETWORK_URL_RE = re.compile(rb"https?://[^\x00\r\n\s\"'<>]+", re.IGNORECASE)
STOCK_BINARY_PATH_RE = re.compile(
    rb"^(?:/{1,2}IDF/components/[A-Za-z0-9._+\-]+(?:/[A-Za-z0-9._+\-]+)*|"
    rb"/builds/idf/crosstool-NG/\.build/riscv32-esp-elf/src/newlib/newlib/"
    rb"libc/stdlib/(?:dtoa|mprec)\.c|"
    rb"//builds/ae_group/esp_ipa/src/esp_ipa_(?:af|awb)\.c|"
    rb"/(?:platform_sd_card\.txt|sdcard|spiffs)|"
    rb"/dev/(?:console|null|secondary|uart(?:/0)?|esps0|video(?:0|20)))$"
)
# Deterministic labels emitted by this workflow or by the pinned ESP32-P4
# precompiled runtime are public source identities, not host filesystem roots.
STOCK_SYNTHETIC_PATH_RE = re.compile(
    rb"^(?:/product/examples/arduino/(?:examples|libraries)/"
    rb"[A-Za-z0-9._+\-/]+|/\./managed_components/"
    rb"(?:espressif__esp_hosted|espressif__esp_video|espressif__esp_wifi_remote)/"
    rb"[A-Za-z0-9._+\-/]+)$"
)
ZIP_LOCAL_FILE_HEADER = b"PK\x03\x04"
ZIP_END_OF_CENTRAL_DIRECTORY = b"PK\x05\x06"
ZIP_LOCAL_HEADER = struct.Struct("<4s5H3L2H")
ZIP_CENTRAL_HEADER = struct.Struct("<4s6H3L5H2L")
ZIP_EOCD = struct.Struct("<4s4H2LH")


class PackagingError(ValueError):
    """Raised when build evidence cannot produce a safe segmented package."""


@dataclass(frozen=True)
class ParsedFlashArgs:
    settings: dict[str, str]
    segments: tuple[tuple[int, str], ...]


@dataclass(frozen=True)
class FlashSegment:
    offset: int
    source_name: str
    source_path: Path
    archive_path: str
    role: str
    size: int
    sha256: str

    @property
    def end(self) -> int:
        return self.offset + self.size


@dataclass(frozen=True)
class FlashLayout:
    settings: dict[str, str]
    capacity_bytes: int
    raw_bytes: bytes
    raw_sha256: str
    segments: tuple[FlashSegment, ...]


@dataclass(frozen=True)
class SourceEvidence:
    basename: str
    size: int
    sha256: str


@dataclass(frozen=True)
class TrustedRepository:
    root: Path
    head: str


@dataclass(frozen=True)
class BuildMetadata:
    resolved_fqbn: str
    board_options: dict[str, str]
    target: str
    options_evidence: SourceEvidence
    compile_commands_evidence: SourceEvidence
    primary_source_evidence: SourceEvidence
    generated_translation_unit_evidence: SourceEvidence
    sketch_object_evidence: SourceEvidence
    application_elf_evidence: SourceEvidence
    link_map_evidence: SourceEvidence
    compile_log_evidence: SourceEvidence
    sketch_object_path: Path
    application_elf_path: Path
    link_map_path: Path
    compile_log_path: Path
    generated_compile_tokens: tuple[str, ...]
    sketch_name: str
    primary_source_basename: str
    generated_translation_unit_basename: str
    application_binary_basename: str
    source_line_count: int
    source_line_mapping_sha256: str
    compile_identity_sha256: str


@dataclass(frozen=True)
class LocalBuildEvidence:
    build_dir: Path
    board_properties: Path
    compile_log: Path


@dataclass(frozen=True)
class BoardMetadata:
    properties_evidence: SourceEvidence
    properties_sha256: str
    sanitized_properties: dict[str, str]
    upload_pattern_sha256: str
    before_reset: str
    after_reset: str
    operation: str
    recipe_segments: tuple[tuple[int, str], ...]
    esptool_binary_sha256: str


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def source_evidence(path: Path, raw_bytes: bytes | None = None) -> SourceEvidence:
    basename = path.name
    if not SAFE_COMPONENT_RE.fullmatch(basename):
        raise PackagingError(f"metadata evidence basename is unsafe: {basename!r}")
    if path.is_symlink() or not path.is_file():
        raise PackagingError(f"metadata evidence must be a regular file: {basename}")
    if raw_bytes is None:
        raw_bytes = path.read_bytes()
    size = len(raw_bytes)
    if size <= 0:
        raise PackagingError(f"metadata evidence is empty: {basename}")
    return SourceEvidence(
        basename=basename,
        size=size,
        sha256=sha256_bytes(raw_bytes),
    )


def evidence_document(evidence: SourceEvidence) -> dict[str, object]:
    return {
        "basename": evidence.basename,
        "size": evidence.size,
        "sha256": evidence.sha256,
    }


def run_git(repository: Path, *arguments: str) -> str:
    clean_environment = {
        key: value for key, value in os.environ.items() if not key.startswith("GIT_")
    }
    clean_environment.update(
        {
            "GIT_NO_REPLACE_OBJECTS": "1",
            "GIT_CONFIG_NOSYSTEM": "1",
            "GIT_CONFIG_GLOBAL": os.devnull,
        }
    )
    result = subprocess.run(
        [
            "git", "-C", str(repository),
            "-c", "core.attributesFile=/dev/null",
            "-c", "core.autocrlf=false",
            "-c", "core.eol=lf",
            *arguments,
        ],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
        env=clean_environment,
    )
    if result.returncode:
        raise PackagingError(
            f"git {' '.join(arguments)} failed ({result.returncode}): "
            f"{result.stderr.strip() or result.stdout.strip()}"
        )
    return result.stdout.strip()


def run_git_with_index(repository: Path, index_path: Path, *arguments: str) -> str:
    """Run Git with a disposable index and all caller GIT_* state removed."""

    environment = {
        key: value for key, value in os.environ.items() if not key.startswith("GIT_")
    }
    environment.update(
        {
            "GIT_INDEX_FILE": str(index_path),
            "GIT_NO_REPLACE_OBJECTS": "1",
            "GIT_CONFIG_NOSYSTEM": "1",
            "GIT_CONFIG_GLOBAL": os.devnull,
        }
    )
    result = subprocess.run(
        [
            "git",
            "-C",
            str(repository),
            "-c",
            "core.fsmonitor=false",
            "-c",
            "core.untrackedCache=false",
            "-c",
            "core.preloadIndex=false",
            "-c",
            "core.trustctime=true",
            "-c",
            "core.attributesFile=/dev/null",
            "-c",
            "core.autocrlf=false",
            "-c",
            "core.eol=lf",
            *arguments,
        ],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
        env=environment,
    )
    if result.returncode:
        raise PackagingError(
            f"isolated git {' '.join(arguments)} failed ({result.returncode}): "
            f"{result.stderr.strip() or result.stdout.strip()}"
        )
    return result.stdout.strip()


def verify_git_configuration(root: Path) -> None:
    """Reject repository-local filtering or replacement mechanisms."""

    git_dir = Path(run_git(root, "rev-parse", "--git-dir"))
    if not git_dir.is_absolute():
        git_dir = (root / git_dir).resolve()
    info_attributes = git_dir / "info" / "attributes"
    if info_attributes.exists() and info_attributes.read_bytes().strip():
        raise PackagingError("Git info/attributes must be empty for trusted packaging")
    replaced = run_git(root, "for-each-ref", "--format=%(refname)", "refs/replace")
    if replaced:
        raise PackagingError("Git replacement refs are forbidden for trusted packaging")
    environment = {
        key: value for key, value in os.environ.items() if not key.startswith("GIT_")
    }
    environment.update({"GIT_CONFIG_NOSYSTEM": "1", "GIT_CONFIG_GLOBAL": os.devnull})
    result = subprocess.run(
        ["git", "-C", str(root), "config", "--local", "--includes", "--null", "--list"],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False, env=environment,
    )
    if result.returncode:
        raise PackagingError("cannot read local Git configuration for trusted packaging")
    entries = result.stdout.decode("utf-8", "strict").split("\x00")
    forbidden = re.compile(r"^filter\.[^.]+\.(?:clean|process|required)(?:\n|=)", re.I)
    if any(forbidden.match(entry) for entry in entries if entry):
        raise PackagingError("Git local clean/process/required filters are forbidden")


def verify_clean_worktree_with_isolated_index(root: Path, head: str) -> None:
    """Content-check the worktree without trusting its real index/stat cache.

    ``git status`` can be made stale by a caller-controlled index option and
    same-size/mtime edits.  Reading HEAD into a private index forces a fresh
    working-tree comparison and leaves the real index entirely untouched.
    """

    with tempfile.TemporaryDirectory(prefix=".arduino-trusted-index-") as temporary:
        index_path = Path(temporary) / "index"
        run_git_with_index(root, index_path, "read-tree", head)
        # Refresh only the disposable index.  This forces Git to examine file
        # content/stat data rather than treating the just-read zero-stat cache
        # entries as a synthetic dirty tree.
        run_git_with_index(root, index_path, "update-index", "--really-refresh", "-q")
        try:
            run_git_with_index(root, index_path, "diff-files", "--quiet", "--")
        except PackagingError as exc:
            raise PackagingError(
                "repository working tree content differs from trusted HEAD"
            ) from exc


def derive_trusted_repository() -> TrustedRepository:
    """Anchor the packager to its own clean Git repository, never caller input.

    The trusted current-repository gate forbids ``--repository-root`` / ``--repo``
    and process-CWD resolution: the only trusted root is derived from this
    script's own ``__file__`` location through Git, and it must be clean with a
    full 40-hex HEAD.
    """

    script_dir = SCRIPT_PATH.parent
    if not script_dir.is_dir():
        raise PackagingError("packager script directory is missing")
    top_level = run_git(script_dir, "rev-parse", "--show-toplevel")
    root = Path(top_level).resolve()
    if not root.is_dir():
        raise PackagingError("derived repository root is not a directory")
    try:
        script_relative = SCRIPT_PATH.relative_to(root).as_posix()
    except ValueError as exc:
        raise PackagingError("packager script is outside its derived Git root") from exc
    if script_relative != PACKAGER_REPOSITORY_PATH.as_posix():
        raise PackagingError(
            "packager must run from its canonical repository-relative location"
        )
    verify_git_configuration(root)
    status = run_git(root, "status", "--porcelain=v1", "--untracked-files=all")
    if status:
        raise PackagingError("repository working tree is not clean")
    index_records = run_git(root, "ls-files", "-v", "-z").split("\x00")
    if any(record and not record.startswith("H ") for record in index_records):
        raise PackagingError(
            "repository index contains assume-unchanged, skip-worktree, or non-cached entries"
        )
    head = run_git(root, "rev-parse", "HEAD")
    if not GIT_SHA_RE.fullmatch(head):
        raise PackagingError("repository HEAD is not a full 40-hex SHA")
    verify_clean_worktree_with_isolated_index(root, head)
    return TrustedRepository(root=root, head=head)


def compute_source_line_mapping(
    generated_text: str,
    primary_source: Path,
) -> tuple[int, str, str]:
    """Bind the generated translation unit's ``#line`` directives to the trusted
    primary sketch source and return the compile-identity fields.

    Every ``#line N "path"`` directive must resolve to the primary ``.ino`` file;
    a forged or stripped directive fails closed. The returned hashes never expose
    the absolute paths themselves.
    """

    try:
        primary_text = primary_source.read_text(encoding="utf-8")
    except UnicodeDecodeError as exc:
        raise PackagingError("primary sketch source is not valid UTF-8") from exc
    source_lines = primary_text.splitlines()
    source_line_count = len(source_lines)
    if source_line_count <= 0:
        raise PackagingError("primary sketch source is empty")
    generated_lines = generated_text.splitlines()
    if generated_lines and generated_lines[-1] == "":
        # Arduino's preprocessor emits one separator newline after the sketch.
        generated_lines.pop()
    directives: list[tuple[int, int]] = []
    for generated_index, raw_line in enumerate(generated_lines):
        if raw_line.startswith("#line ") and not LINE_DIRECTIVE_RE.fullmatch(raw_line):
            raise PackagingError("generated translation unit has a non-canonical #line directive")
        match = LINE_DIRECTIVE_RE.match(raw_line)
        if not match:
            continue
        line_number = int(match.group(1))
        if line_number > source_line_count:
            raise PackagingError(
                "generated translation unit #line directive exceeds the primary source"
            )
        referenced = match.group(2)
        try:
            resolved = Path(referenced).resolve(strict=True)
        except FileNotFoundError as exc:
            raise PackagingError(
                "generated translation unit #line directive references a missing file"
            ) from exc
        if resolved != primary_source.resolve():
            raise PackagingError(
                "generated translation unit #line directive does not reference the "
                "trusted primary sketch source"
            )
        directives.append((generated_index, line_number))
    if not directives:
        raise PackagingError(
            "generated translation unit has no #line directive bound to the primary source"
        )
    first_index, first_source_line = directives[0]
    if first_source_line != 1:
        raise PackagingError("generated translation unit must begin its source mapping at line 1")
    if generated_lines[:first_index] not in ([], ["#include <Arduino.h>"]):
        raise PackagingError("generated translation unit has a non-canonical preamble")

    if len(directives) == 1:
        if generated_lines[first_index + 1 :] != source_lines:
            raise PackagingError(
                "generated translation unit does not reproduce the primary sketch source"
            )
    else:
        body_index, body_source_line = directives[-1]
        prototype_directives = directives[1:-1]
        if not prototype_directives or body_source_line != min(
            line_number for _index, line_number in prototype_directives
        ):
            raise PackagingError(
                "generated translation unit has a non-canonical prototype/source boundary"
            )
        first_prototype_index = prototype_directives[0][0]
        if generated_lines[first_index + 1 : first_prototype_index] != source_lines[
            : body_source_line - 1
        ]:
            raise PackagingError(
                "generated translation unit preamble differs from the primary sketch source"
            )
        for directive_position, (prototype_index, prototype_source_line) in enumerate(
            prototype_directives, start=1
        ):
            next_index = directives[directive_position + 1][0]
            prototype_lines = generated_lines[prototype_index + 1 : next_index]
            source_line = source_lines[prototype_source_line - 1]
            brace_index = source_line.find("{")
            if brace_index <= 0:
                raise PackagingError(
                    "generated translation unit prototype source line is not a canonical definition"
                )
            expected_prototype = source_line[:brace_index].strip() + ";"
            if (
                len(prototype_lines) != 1
                or prototype_lines[0] != expected_prototype
            ):
                raise PackagingError(
                    "generated translation unit has a non-canonical injected prototype"
                )
        if generated_lines[body_index + 1 :] != source_lines[body_source_line - 1 :]:
            raise PackagingError(
                "generated translation unit body differs from the primary sketch source"
            )

    mapping_document = json.dumps(
        [
            {"generated_line": generated_index + 1, "source_line": source_line}
            for generated_index, source_line in directives
        ],
        sort_keys=True,
    )
    source_line_mapping_sha256 = sha256_bytes(mapping_document.encode("utf-8"))
    compile_identity_sha256 = sha256_bytes(
        json.dumps(
            {
                "primary_source_sha256": sha256_bytes(primary_text.encode("utf-8")),
                "generated_translation_unit_sha256": sha256_bytes(
                    generated_text.encode("utf-8")
                ),
                "source_line_mapping_sha256": source_line_mapping_sha256,
            },
            sort_keys=True,
        ).encode("utf-8")
    )
    return source_line_count, source_line_mapping_sha256, compile_identity_sha256


def validate_archive_name(name: str) -> str:
    """Return a safe normalized ZIP member path or fail closed."""

    if not name or "\\" in name or "\x00" in name:
        raise PackagingError(f"unsafe ZIP member path: {name!r}")
    pure = PurePosixPath(name)
    if pure.is_absolute() or any(part in {"", ".", ".."} for part in pure.parts):
        raise PackagingError(f"unsafe ZIP member path: {name!r}")
    normalized = pure.as_posix()
    if normalized != name or normalized.endswith("/"):
        raise PackagingError(f"non-canonical ZIP member path: {name!r}")
    return normalized


def validate_source_name(name: str) -> str:
    """Validate a build-relative file name that is also safe in both helpers."""

    if "\\" in name:
        raise PackagingError(f"flash_args file name must use forward slashes: {name!r}")
    normalized = validate_archive_name(name)
    if not all(SAFE_COMPONENT_RE.fullmatch(part) for part in PurePosixPath(normalized).parts):
        raise PackagingError(f"flash_args file name is not helper-safe: {name!r}")
    return normalized


def parse_flash_size(value: str) -> int:
    match = FLASH_SIZE_RE.fullmatch(value)
    if not match:
        raise PackagingError(f"unsupported flash size in flash_args: {value!r}")
    number = int(match.group(1), 0)
    unit = (match.group(2) or "").upper()
    factors = {"": 1, "KB": 1024, "KIB": 1024, "MB": 1024**2, "MIB": 1024**2,
               "GB": 1024**3, "GIB": 1024**3}
    capacity = number * factors[unit]
    if capacity <= 0:
        raise PackagingError("flash capacity must be positive")
    return capacity


def parse_flash_args_text(text: str) -> ParsedFlashArgs:
    """Parse the Arduino core's generated flash_args without board offsets."""

    try:
        tokens = shlex.split(text, comments=False, posix=True)
    except ValueError as exc:
        raise PackagingError(f"cannot parse flash_args: {exc}") from exc
    if not tokens:
        raise PackagingError("flash_args is empty")

    settings: dict[str, str] = {}
    segments: list[tuple[int, str]] = []
    index = 0
    while index < len(tokens):
        token = tokens[index]
        if token.startswith("-"):
            if token not in FLASH_SETTING_FLAGS:
                raise PackagingError(f"unsupported flash_args option: {token!r}")
            if token in settings:
                raise PackagingError(f"duplicate flash_args option: {token}")
            if index + 1 >= len(tokens):
                raise PackagingError(f"missing value for flash_args option: {token}")
            value = tokens[index + 1]
            if not SAFE_VALUE_RE.fullmatch(value):
                raise PackagingError(f"unsafe value for {token}: {value!r}")
            settings[token] = value
            index += 2
            continue

        if index + 1 >= len(tokens):
            raise PackagingError(f"missing binary after flash offset: {token!r}")
        if not re.fullmatch(r"(?:0[xX][0-9a-fA-F]+|[0-9]+)", token):
            raise PackagingError(f"invalid flash offset: {token!r}")
        offset = int(token, 0)
        source_name = validate_source_name(tokens[index + 1])
        segments.append((offset, source_name))
        index += 2

    missing = sorted(set(FLASH_SETTING_FLAGS) - set(settings))
    if missing:
        raise PackagingError("flash_args is missing required settings: " + ", ".join(missing))
    if not segments:
        raise PackagingError("flash_args contains no binary segments")
    return ParsedFlashArgs(settings=settings, segments=tuple(segments))


def resolve_build_file(build_dir: Path, relative_name: str) -> Path:
    candidate = build_dir.joinpath(*PurePosixPath(relative_name).parts)
    if candidate.is_symlink():
        raise PackagingError(f"build segment must not be a symlink: {relative_name}")
    try:
        resolved = candidate.resolve(strict=True)
        resolved.relative_to(build_dir)
    except (FileNotFoundError, ValueError) as exc:
        raise PackagingError(
            f"build segment is missing or escapes build directory: {relative_name}"
        ) from exc
    if not resolved.is_file() or not stat.S_ISREG(resolved.stat().st_mode):
        raise PackagingError(f"build segment is not a regular file: {relative_name}")
    return resolved


def segment_role(source_name: str, application_binary_basename: str) -> str:
    name = PurePosixPath(source_name).name.lower()
    if name == "boot_app0.bin":
        return "boot_app0"
    if "bootloader" in name:
        return "bootloader"
    if "partition" in name:
        return "partition_table"
    if source_name == application_binary_basename:
        return "application"
    return "other"


def load_flash_layout(build_dir: Path, application_binary_basename: str) -> FlashLayout:
    try:
        build_dir = build_dir.resolve(strict=True)
    except FileNotFoundError as exc:
        raise PackagingError(f"build directory does not exist: {build_dir}") from exc
    if not build_dir.is_dir():
        raise PackagingError(f"build path is not a directory: {build_dir}")

    flash_args_path = build_dir / "flash_args"
    if flash_args_path.is_symlink() or not flash_args_path.is_file():
        raise PackagingError("Arduino build output is missing a regular flash_args file")
    raw_bytes = flash_args_path.read_bytes()
    try:
        parsed = parse_flash_args_text(raw_bytes.decode("utf-8"))
    except UnicodeDecodeError as exc:
        raise PackagingError("flash_args is not valid UTF-8") from exc
    capacity = parse_flash_size(parsed.settings["--flash-size"])

    segments: list[FlashSegment] = []
    offsets: set[int] = set()
    source_names: set[str] = set()
    archive_paths: set[str] = set()
    for offset, source_name in parsed.segments:
        if offset in offsets:
            raise PackagingError(f"duplicate flash offset: {offset:#x}")
        if source_name in source_names:
            raise PackagingError(f"duplicate flash segment file: {source_name}")
        source_path = resolve_build_file(build_dir, source_name)
        archive_path = validate_archive_name(f"bin/{source_name}")
        if archive_path in archive_paths:
            raise PackagingError(f"duplicate archive segment path: {archive_path}")
        size = source_path.stat().st_size
        if size <= 0:
            raise PackagingError(f"flash segment is empty: {source_name}")
        lowered = PurePosixPath(source_name).name.lower()
        if "merged" in lowered or "whole-flash" in lowered or "whole_flash" in lowered:
            raise PackagingError(
                f"merged/whole-flash images are not segmented inputs: {source_name}"
            )
        if offset == 0 and size == capacity:
            raise PackagingError(
                f"whole-flash image at 0x0 is forbidden as a segmented artifact: {source_name}"
            )
        if offset + size > capacity:
            raise PackagingError(
                f"flash segment exceeds capacity: {source_name} ends at {offset + size:#x}, "
                f"capacity is {capacity:#x}"
            )
        segments.append(
            FlashSegment(
                offset=offset,
                source_name=source_name,
                source_path=source_path,
                archive_path=archive_path,
                role=segment_role(source_name, application_binary_basename),
                size=size,
                sha256=sha256_file(source_path),
            )
        )
        offsets.add(offset)
        source_names.add(source_name)
        archive_paths.add(archive_path)

    ordered = sorted(segments, key=lambda item: item.offset)
    for previous, current in zip(ordered, ordered[1:]):
        if current.offset < previous.end:
            raise PackagingError(
                f"flash segments overlap: {previous.source_name} ends at {previous.end:#x}, "
                f"{current.source_name} starts at {current.offset:#x}"
            )
    required_roles = {"bootloader", "partition_table", "application"}
    actual_roles = {segment.role for segment in segments}
    if not required_roles.issubset(actual_roles):
        raise PackagingError(
            "flash_args does not describe a complete segmented Arduino image; missing roles: "
            + ", ".join(sorted(required_roles - actual_roles))
        )
    if sum(segment.size for segment in segments) >= capacity:
        raise PackagingError(
            "segmented payload total must be strictly smaller than flash capacity"
        )
    return FlashLayout(
        settings=dict(parsed.settings),
        capacity_bytes=capacity,
        raw_bytes=raw_bytes,
        raw_sha256=sha256_bytes(raw_bytes),
        segments=tuple(segments),
    )


def parse_board_options(value: str) -> dict[str, str]:
    options: dict[str, str] = {}
    if not value:
        return options
    for item in value.split(","):
        if item.count("=") != 1:
            raise PackagingError(f"invalid board option: {item!r}")
        key, option_value = item.split("=", 1)
        if not SAFE_COMPONENT_RE.fullmatch(key) or not SAFE_VALUE_RE.fullmatch(option_value):
            raise PackagingError(f"unsafe board option: {item!r}")
        if key in options:
            raise PackagingError(f"duplicate board option: {key}")
        options[key] = option_value
    return options


def resolve_repository_sketch(
    repository_root: Path, declared_sketch_path: str
) -> tuple[Path, PurePosixPath]:
    declared = PurePosixPath(validate_archive_name(declared_sketch_path))
    try:
        root = repository_root.resolve(strict=True)
    except FileNotFoundError as exc:
        raise PackagingError("repository root does not exist") from exc
    if not root.is_dir():
        raise PackagingError("repository root is not a directory")
    candidate = root.joinpath(*declared.parts)
    try:
        resolved = candidate.resolve(strict=True)
        resolved.relative_to(root)
    except (FileNotFoundError, ValueError) as exc:
        raise PackagingError("declared sketch path is missing or escapes repository root") from exc
    if candidate.is_symlink() or not resolved.is_dir():
        raise PackagingError("declared sketch path must be a real repository directory")
    return resolved, declared


def load_compile_identity(
    build_dir: Path, sketch_name: str, repository_root: Path
) -> tuple[Path, Path, SourceEvidence, SourceEvidence, tuple[str, ...]]:
    commands_path = build_dir / "compile_commands.json"
    generated_path = build_dir / "sketch" / f"{sketch_name}.ino.cpp"
    if commands_path.is_symlink() or not commands_path.is_file():
        raise PackagingError("Arduino build output is missing compile_commands.json")
    if generated_path.is_symlink() or not generated_path.is_file():
        raise PackagingError(
            "Arduino build output is missing the generated sketch translation unit"
        )
    raw = commands_path.read_bytes()
    try:
        document = json.loads(raw.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise PackagingError(f"cannot parse compile_commands.json: {exc}") from exc
    if not isinstance(document, list) or not document:
        raise PackagingError("compile_commands.json must contain compile entries")
    expected_generated = generated_path.resolve(strict=True)
    expected_object = (build_dir / "sketch" / f"{sketch_name}.ino.cpp.o").resolve()
    matches = 0
    matched_tokens: tuple[str, ...] | None = None
    for entry in document:
        if not isinstance(entry, dict):
            raise PackagingError("compile_commands.json contains a non-object entry")
        file_value = entry.get("file")
        directory_value = entry.get("directory")
        if not isinstance(file_value, str) or not isinstance(directory_value, str):
            raise PackagingError("compile_commands.json entry lacks file/directory identity")
        try:
            entry_directory = Path(directory_value).resolve(strict=True)
        except FileNotFoundError as exc:
            raise PackagingError(
                "compile_commands.json entry references a missing directory"
            ) from exc
        if entry_directory != repository_root.resolve(strict=True):
            raise PackagingError(
                "compile_commands.json entry directory differs from the trusted repository root"
            )
        entry_file = Path(file_value)
        if not entry_file.is_absolute():
            entry_file = entry_directory / entry_file
        try:
            resolved_file = entry_file.resolve(strict=True)
        except FileNotFoundError:
            continue
        if resolved_file == expected_generated:
            arguments_value = entry.get("arguments")
            command_value = entry.get("command")
            has_arguments = "arguments" in entry
            has_command = "command" in entry
            if has_arguments == has_command:
                raise PackagingError(
                    "compile_commands.json entry must use exactly one command representation"
                )
            if has_arguments and isinstance(arguments_value, list):
                if not arguments_value or not all(
                    isinstance(value, str) and "\x00" not in value
                    for value in arguments_value
                ):
                    raise PackagingError(
                        "compile_commands.json arguments are empty or unsafe"
                    )
                compile_tokens = list(arguments_value)
            elif has_command and isinstance(command_value, str) and command_value:
                try:
                    compile_tokens = shlex.split(command_value, comments=False, posix=True)
                except ValueError as exc:
                    raise PackagingError(
                        f"cannot parse compile_commands.json command: {exc}"
                    ) from exc
            else:
                raise PackagingError(
                    "compile_commands.json entry lacks a usable command/arguments identity"
                )
            if compile_tokens.count("-c") != 1:
                raise PackagingError(
                    "compile command must contain exactly one compile-only flag"
                )
            if compile_tokens.count("-o") != 1:
                raise PackagingError(
                    "compile command must contain exactly one output option"
                )
            output_index = compile_tokens.index("-o")
            source_index = output_index - 1
            if (
                source_index <= compile_tokens.index("-c")
                or output_index + 2 != len(compile_tokens)
            ):
                raise PackagingError(
                    "compile command does not use the canonical Arduino source/output layout"
                )
            source_token = Path(compile_tokens[source_index])
            if not source_token.is_absolute():
                source_token = entry_directory / source_token
            try:
                resolved_source_token = source_token.resolve(strict=True)
            except (FileNotFoundError, OSError) as exc:
                raise PackagingError("compile command source is missing") from exc
            if resolved_source_token != expected_generated:
                raise PackagingError(
                    "compile command does not compile the expected generated sketch "
                    "translation unit"
                )
            for token in compile_tokens[compile_tokens.index("-c") + 1 : source_index]:
                if not token or token.startswith(("-", "@")):
                    continue
                candidate = Path(token)
                if candidate.suffix.lower() not in {
                    ".c",
                    ".cc",
                    ".cpp",
                    ".cxx",
                    ".ino",
                    ".s",
                    ".sx",
                }:
                    continue
                if not candidate.is_absolute():
                    candidate = entry_directory / candidate
                if candidate.exists():
                    raise PackagingError(
                        "compile command contains an unexpected additional source input"
                    )
            output_token = Path(compile_tokens[output_index + 1])
            if not output_token.is_absolute():
                output_token = entry_directory / output_token
            expected_output = build_dir / "sketch" / f"{sketch_name}.ino.cpp.o"
            try:
                resolved_output = output_token.resolve(strict=True)
                resolved_output.relative_to(build_dir.resolve(strict=True))
            except (FileNotFoundError, ValueError, OSError) as exc:
                raise PackagingError(
                    "compile command output is missing or escapes the build directory"
                ) from exc
            if (
                output_token.is_symlink()
                or not resolved_output.is_file()
                or not stat.S_ISREG(resolved_output.stat().st_mode)
                or resolved_output != expected_output.resolve(strict=True)
            ):
                raise PackagingError(
                    "compile command output is not the canonical generated sketch object"
                )
            matches += 1
            matched_tokens = tuple(compile_tokens)
    if matches != 1:
        raise PackagingError(
            "compile_commands.json must contain exactly one entry for the generated sketch "
            "translation unit"
        )
    if matched_tokens is None:
        raise PackagingError("compile_commands.json did not retain generated sketch tokens")
    return (
        expected_generated,
        expected_object,
        source_evidence(commands_path, raw),
        source_evidence(expected_generated),
        matched_tokens,
    )


def load_build_metadata(
    build_dir: Path,
    repository_root: Path,
    declared_sketch_path: str,
    requested_fqbn: str,
    requested_board_options: str,
    compile_log_path: Path,
) -> BuildMetadata:
    if not FQBN_RE.fullmatch(requested_fqbn):
        raise PackagingError(f"invalid FQBN: {requested_fqbn!r}")
    options_path = build_dir / "build.options.json"
    sdkconfig_path = build_dir / "sdkconfig"
    if options_path.is_symlink() or not options_path.is_file():
        raise PackagingError("Arduino build output is missing build.options.json")
    if sdkconfig_path.is_symlink() or not sdkconfig_path.is_file():
        raise PackagingError("Arduino build output is missing sdkconfig")
    options_raw = options_path.read_bytes()
    try:
        options_document = json.loads(options_raw.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise PackagingError(f"cannot parse build.options.json: {exc}") from exc
    if not isinstance(options_document, dict):
        raise PackagingError("build.options.json must contain an object")
    resolved_fqbn = options_document.get("fqbn")
    if not isinstance(resolved_fqbn, str):
        raise PackagingError("build.options.json has no string fqbn")
    actual_parts = resolved_fqbn.split(":", 3)
    if len(actual_parts) < 3:
        raise PackagingError(f"build.options.json contains invalid fqbn: {resolved_fqbn!r}")
    actual_base = ":".join(actual_parts[:3])
    actual_option_text = actual_parts[3] if len(actual_parts) == 4 else ""
    requested_options = parse_board_options(requested_board_options)
    actual_options = parse_board_options(actual_option_text)
    if actual_base != requested_fqbn:
        raise PackagingError(
            f"requested FQBN {requested_fqbn!r} does not match build metadata {actual_base!r}"
        )
    if actual_options != requested_options:
        raise PackagingError(
            "requested board options do not match build metadata: "
            f"requested={requested_options!r} actual={actual_options!r}"
        )
    sketch_dir, canonical_sketch_path = resolve_repository_sketch(
        repository_root, declared_sketch_path
    )
    sketch_name = canonical_sketch_path.name
    if not SAFE_COMPONENT_RE.fullmatch(sketch_name):
        raise PackagingError("declared sketch directory name is unsafe")
    sketch_location = options_document.get("sketchLocation")
    if not isinstance(sketch_location, str) or not Path(sketch_location).is_absolute():
        raise PackagingError("build.options.json has no absolute sketchLocation")
    try:
        built_sketch_dir = Path(sketch_location).resolve(strict=True)
    except FileNotFoundError as exc:
        raise PackagingError("build.options.json sketchLocation does not exist") from exc
    if built_sketch_dir != sketch_dir:
        raise PackagingError(
            "declared sketch path does not match build.options.json sketchLocation"
        )
    primary_source_basename = f"{sketch_name}.ino"
    primary_source = sketch_dir / primary_source_basename
    if primary_source.is_symlink() or not primary_source.is_file():
        raise PackagingError(
            "declared sketch is missing its directory-named primary .ino source"
        )
    generated_path, object_path, commands_evidence, generated_evidence, compile_tokens = load_compile_identity(
        build_dir, sketch_name, repository_root
    )
    application_path = build_dir / f"{sketch_name}.ino.bin"
    elf_path = build_dir / f"{sketch_name}.ino.elf"
    map_path = build_dir / f"{sketch_name}.ino.map"
    for label, evidence_path in (
        ("sketch object", object_path),
        ("application ELF", elf_path),
        ("link map", map_path),
        ("compile log", compile_log_path),
    ):
        if evidence_path.is_symlink() or not evidence_path.is_file():
            raise PackagingError(f"Arduino build output is missing a regular {label}")
    if application_path.is_symlink() or not application_path.is_file():
        raise PackagingError("Arduino build output is missing the application binary")
    try:
        sdkconfig_text = sdkconfig_path.read_text(encoding="utf-8")
    except UnicodeDecodeError as exc:
        raise PackagingError("sdkconfig is not valid UTF-8") from exc
    target_match = SDKCONFIG_TARGET_RE.search(sdkconfig_text)
    if not target_match:
        raise PackagingError("sdkconfig does not declare CONFIG_IDF_TARGET")
    target = target_match.group(1)
    if not TARGET_RE.fullmatch(target):
        raise PackagingError(f"unsafe build target: {target!r}")
    try:
        generated_text = generated_path.read_text(encoding="utf-8")
    except UnicodeDecodeError as exc:
        raise PackagingError("generated translation unit is not valid UTF-8") from exc
    source_line_count, source_line_mapping_sha256, compile_identity_sha256 = (
        compute_source_line_mapping(generated_text, primary_source)
    )
    return BuildMetadata(
        resolved_fqbn=resolved_fqbn,
        board_options=actual_options,
        target=target,
        options_evidence=source_evidence(options_path, options_raw),
        compile_commands_evidence=commands_evidence,
        primary_source_evidence=source_evidence(primary_source),
        generated_translation_unit_evidence=generated_evidence,
        sketch_object_evidence=source_evidence(object_path),
        application_elf_evidence=source_evidence(elf_path),
        link_map_evidence=source_evidence(map_path),
        compile_log_evidence=source_evidence(compile_log_path),
        sketch_object_path=object_path,
        application_elf_path=elf_path,
        link_map_path=map_path,
        compile_log_path=compile_log_path.resolve(strict=True),
        generated_compile_tokens=compile_tokens,
        sketch_name=sketch_name,
        primary_source_basename=primary_source_basename,
        generated_translation_unit_basename=generated_path.name,
        application_binary_basename=f"{sketch_name}.ino.bin",
        source_line_count=source_line_count,
        source_line_mapping_sha256=source_line_mapping_sha256,
        compile_identity_sha256=compile_identity_sha256,
    )


def parse_properties(path: Path) -> tuple[dict[str, str], bytes]:
    if path.is_symlink() or not path.is_file():
        raise PackagingError("resolved board-properties input must be a regular file")
    raw = path.read_bytes()
    try:
        lines = raw.decode("utf-8").splitlines()
    except UnicodeDecodeError as exc:
        raise PackagingError("resolved board properties are not valid UTF-8") from exc
    properties: dict[str, str] = {}
    for line_number, line in enumerate(lines, start=1):
        if not line:
            continue
        if "=" not in line:
            raise PackagingError(f"invalid board-properties line {line_number}")
        key, value = line.split("=", 1)
        if not key or key in properties:
            raise PackagingError(f"duplicate or empty board property at line {line_number}")
        properties[key] = value
    return properties, raw


def recipe_value(tokens: list[str], flag: str) -> str:
    if tokens.count(flag) != 1:
        raise PackagingError(f"resolved upload recipe must contain {flag} exactly once")
    index = tokens.index(flag)
    if index + 1 >= len(tokens):
        raise PackagingError(f"resolved upload recipe is missing a value for {flag}")
    value = tokens[index + 1]
    if not SAFE_VALUE_RE.fullmatch(value):
        raise PackagingError(f"resolved upload recipe has unsafe {flag} value: {value!r}")
    return value


def parse_recipe_segments(
    tokens: list[str],
    operation_index: int,
    build_dir: Path,
    platform_root: Path,
    build_metadata: BuildMetadata,
    layout: FlashLayout,
) -> tuple[tuple[int, str], ...]:
    """Resolve the upload recipe's real offset/file pairs without publishing paths."""

    offset_re = re.compile(r"(?:0[xX][0-9a-fA-F]+|[0-9]+)")
    flag_only = {"-z", "--compress", "--no-compress", "--no-stub"}
    raw_pairs: list[tuple[int, str]] = []
    index = operation_index + 1
    seen_segment = False
    while index < len(tokens):
        token = tokens[index]
        if offset_re.fullmatch(token):
            if index + 1 >= len(tokens):
                raise PackagingError("resolved upload recipe has an offset without a file")
            raw_pairs.append((int(token, 0), tokens[index + 1]))
            seen_segment = True
            index += 2
            continue
        if seen_segment:
            raise PackagingError("resolved upload recipe has non-pair data after flash segments")
        if token in flag_only:
            index += 1
            continue
        if token.startswith("-"):
            if index + 1 >= len(tokens) or tokens[index + 1].startswith("-"):
                raise PackagingError(
                    f"resolved upload recipe option has no value: {token!r}"
                )
            index += 2
            continue
        raise PackagingError(f"unsupported resolved upload recipe token: {token!r}")

    if not raw_pairs:
        raise PackagingError("resolved upload recipe contains no flash segments")
    expected_project_name = f"{build_metadata.sketch_name}.ino"
    canonical_pairs: list[tuple[int, str]] = []
    for (offset, recipe_path_text), segment in zip(raw_pairs, layout.segments):
        expanded = recipe_path_text.replace("{build.path}", build_dir.as_posix()).replace(
            "{build.project_name}", expected_project_name
        )
        if "{" in expanded or "}" in expanded:
            raise PackagingError("resolved upload recipe segment retains an unknown placeholder")
        recipe_path = Path(expanded)
        if not recipe_path.is_absolute():
            recipe_path = build_dir / recipe_path
        if recipe_path.is_symlink() or not recipe_path.is_file():
            raise PackagingError("resolved upload recipe segment is missing or not a regular file")
        try:
            resolved_recipe_path = recipe_path.resolve(strict=True)
        except FileNotFoundError as exc:
            raise PackagingError("resolved upload recipe segment is missing") from exc
        resolved_build_source = segment.source_path.resolve(strict=True)
        if resolved_recipe_path != resolved_build_source:
            try:
                resolved_recipe_path.relative_to(build_dir.resolve(strict=True))
            except ValueError:
                pass
            else:
                raise PackagingError(
                    "resolved upload recipe segment is not the flash_args build file"
                )
            expected_core_boot_app0 = (
                platform_root / "tools" / "partitions" / "boot_app0.bin"
            )
            core_chain = (
                platform_root / "tools",
                platform_root / "tools" / "partitions",
                expected_core_boot_app0,
            )
            if any(path.is_symlink() for path in core_chain):
                raise PackagingError(
                    "resolved Arduino core boot_app0 path contains a symlink"
                )
            try:
                expected_core_boot_app0 = expected_core_boot_app0.resolve(strict=True)
                expected_core_boot_app0.relative_to(platform_root.resolve(strict=True))
            except FileNotFoundError as exc:
                raise PackagingError(
                    "resolved Arduino core boot_app0 source is missing"
                ) from exc
            except ValueError as exc:
                raise PackagingError(
                    "resolved Arduino core boot_app0 source escapes platform root"
                ) from exc
            if (
                segment.role != "boot_app0"
                or segment.source_name != "boot_app0.bin"
                or recipe_path.is_symlink()
                or resolved_recipe_path != expected_core_boot_app0
                or not resolved_recipe_path.is_file()
                or not stat.S_ISREG(resolved_recipe_path.stat().st_mode)
            ):
                raise PackagingError(
                    "resolved upload recipe segment escapes the exact build directory"
                )
        basename = resolved_recipe_path.name
        if not SAFE_COMPONENT_RE.fullmatch(basename):
            raise PackagingError("resolved upload recipe segment basename is unsafe")
        if (offset, basename) != (segment.offset, segment.source_name):
            raise PackagingError(
                "core-generated flash_args offset/file pairs differ from resolved upload recipe"
            )
        if (
            resolved_recipe_path.stat().st_size != segment.size
            or sha256_file(resolved_recipe_path) != segment.sha256
        ):
            raise PackagingError(
                "resolved upload recipe segment bytes differ from core-generated build output"
            )
        canonical_pairs.append((offset, basename))
    if len(raw_pairs) != len(layout.segments):
        raise PackagingError(
            "core-generated flash_args segment count differs from resolved upload recipe"
        )
    return tuple(canonical_pairs)


def read_esptool_version(binary: Path) -> tuple[str, str]:
    if binary.is_symlink() or not binary.is_file() or not os.access(binary, os.X_OK):
        raise PackagingError("resolved Arduino esptool binary is missing or not executable")
    try:
        result = subprocess.run(
            [str(binary), "version"],
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=15,
        )
    except (OSError, subprocess.TimeoutExpired) as exc:
        raise PackagingError(f"cannot execute resolved Arduino esptool: {exc}") from exc
    if result.returncode:
        raise PackagingError(
            f"resolved Arduino esptool version command failed with {result.returncode}"
        )
    matches = re.findall(r"(?m)^([0-9]+\.[0-9]+\.[0-9]+)$", result.stdout)
    if len(matches) != 1:
        raise PackagingError("resolved Arduino esptool did not report one exact semantic version")
    return matches[0], sha256_file(binary)


def _command_lines(path: Path) -> list[list[str]]:
    """Parse only complete, directly-executable verbose Arduino commands."""

    try:
        text = path.read_text(encoding="utf-8")
    except UnicodeDecodeError as exc:
        raise PackagingError("compile log is not valid UTF-8") from exc
    commands: list[list[str]] = []
    for raw_line in text.splitlines():
        if not raw_line or raw_line[0].isspace():
            continue
        try:
            tokens = shlex.split(raw_line, comments=False, posix=True)
        except ValueError:
            continue
        if tokens:
            commands.append(tokens)
    if not commands:
        raise PackagingError("compile log contains no parseable verbose commands")
    return commands


def _regular_resolved(path: Path, label: str) -> Path:
    if path.is_symlink() or not path.is_file():
        raise PackagingError(f"{label} must be a regular file")
    try:
        return path.resolve(strict=True)
    except (FileNotFoundError, OSError) as exc:
        raise PackagingError(f"{label} is missing") from exc


def _trusted_compiler(properties: dict[str, str]) -> Path:
    root_text = properties["runtime.tools.esp-rv32.path"]
    compiler_name = properties["compiler.cpp.cmd"]
    if not Path(root_text).is_absolute() or not SAFE_VALUE_RE.fullmatch(compiler_name):
        raise PackagingError("resolved RISC-V compiler identity is unsafe")
    root = Path(root_text)
    if root.is_symlink() or not root.is_dir():
        raise PackagingError("resolved RISC-V compiler root is not a regular directory")
    compiler = root / "bin" / compiler_name
    compiler = _regular_resolved(compiler, "resolved RISC-V compiler")
    try:
        compiler.relative_to(root.resolve(strict=True) / "bin")
    except ValueError as exc:
        raise PackagingError("resolved compiler escapes runtime.tools.esp-rv32.path/bin") from exc
    if not os.access(compiler, os.X_OK):
        raise PackagingError("resolved RISC-V compiler is not executable")
    return compiler


def _path_token(value: str, repository_root: Path) -> Path:
    path = Path(value)
    return (path if path.is_absolute() else repository_root / path).resolve()


def _replace_output(tokens: list[str], old: Path, new: Path) -> list[str]:
    replaced = list(tokens)
    old_text = old.as_posix()
    new_text = new.as_posix()
    for index, token in enumerate(replaced):
        if token == old_text:
            replaced[index] = new_text
        elif token.startswith("-Wl,--Map=") and token.removeprefix("-Wl,--Map=") == old_text:
            replaced[index] = "-Wl,--Map=" + new_text
    return replaced


def _run_replay(tokens: list[str], label: str, repository_root: Path) -> None:
    try:
        replay_cwd = repository_root.resolve(strict=True)
    except FileNotFoundError as exc:
        raise PackagingError("trusted repository root is missing for replay") from exc
    if not replay_cwd.is_dir():
        raise PackagingError("trusted repository root is not a directory for replay")
    try:
        result = subprocess.run(
            tokens, cwd=replay_cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            check=False, timeout=180,
        )
    except (OSError, subprocess.TimeoutExpired) as exc:
        raise PackagingError(f"cannot replay {label}: {exc}") from exc
    if result.returncode:
        raise PackagingError(f"{label} replay failed with {result.returncode}")


def _require_esptool_image_info(
    esptool: Path, target: str, application: Path, elf: Path, repository_root: Path
) -> None:
    try:
        result = subprocess.run(
            [str(esptool), "--chip", target, "image-info", str(application)],
            text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            check=False, timeout=30, cwd=repository_root,
        )
    except (OSError, subprocess.TimeoutExpired) as exc:
        raise PackagingError(f"cannot inspect Arduino application image: {exc}") from exc
    if result.returncode:
        raise PackagingError("esptool image-info rejected the Arduino application image")
    elf_hashes = re.findall(r"(?mi)^ELF file SHA256:\s*([0-9a-f]{64})\s*$", result.stdout)
    if len(elf_hashes) != 1 or elf_hashes[0].lower() != sha256_file(elf):
        raise PackagingError("application image ELF SHA-256 does not match the linked ELF")
    if not re.search(r"(?mi)^Checksum:.*\(valid\)\s*$", result.stdout) or not re.search(
        r"(?mi)^Validation hash:.*\(valid\)\s*$", result.stdout
    ):
        raise PackagingError("application image does not have valid esptool checksum/hash evidence")


def canonical_link_map_bytes(raw: bytes, expected_elf: Path) -> bytes:
    """Normalize precisely one linker OUTPUT path, and nothing else.

    GNU ld records the selected ``-o`` path in its map.  A replay necessarily
    changes that one path; every other byte remains provenance evidence.
    """

    try:
        elf_bytes = expected_elf.resolve(strict=True).as_posix().encode("utf-8")
    except (FileNotFoundError, OSError) as exc:
        raise PackagingError("link-map expected ELF is missing") from exc
    pattern = re.compile(
        rb"(?m)^OUTPUT\(" + re.escape(elf_bytes) + rb" elf32-littleriscv\)\r?$"
    )
    matches = list(pattern.finditer(raw))
    if len(matches) != 1:
        raise PackagingError("link map must have exactly one canonical OUTPUT(expected ELF) line")
    match = matches[0]
    normalized = b"OUTPUT(<REPLAYED-ELF> elf32-littleriscv)"
    if match.group(0).endswith(b"\r"):
        normalized += b"\r"
    return raw[:match.start()] + normalized + raw[match.end():]


def recipe_whole_archive_controls(tokens: list[str]) -> set[str]:
    """Return protected linker controls encoded in a compiler recipe token list."""

    def protected(component: str) -> bool:
        if not component.startswith("-"):
            return False
        normalized = component.lstrip("-")
        return (
            len(normalized) >= 2 and "whole-archive".startswith(normalized)
        ) or (
            len(normalized) >= 5 and "no-whole-archive".startswith(normalized)
        )

    found: set[str] = set()
    for index, token in enumerate(tokens):
        if protected(token):
            found.add(token)
        elif token.startswith("-Wl,"):
            found.update(
                component
                for component in token.removeprefix("-Wl,").split(",")
                if protected(component)
            )
        elif token == "-Xlinker" and index + 1 < len(tokens):
            if protected(tokens[index + 1]):
                found.add(tokens[index + 1])
        elif token.startswith("-Xlinker="):
            component = token.removeprefix("-Xlinker=")
            if protected(component):
                found.add(component)
    return found


def verify_source_to_binary_provenance(
    properties: dict[str, str],
    build_dir: Path,
    build_metadata: BuildMetadata,
    target: str,
    esptool: Path,
    repository_root: Path,
) -> None:
    """Replay Arduino's exact compile, link and objcopy operations fail-closed."""

    required = {
        "runtime.tools.esp-rv32.path", "compiler.cpp.cmd",
        "recipe.c.combine.pattern", "recipe.objcopy.bin.pattern_args",
    }
    if missing := sorted(required - set(properties)):
        raise PackagingError("resolved board properties are missing provenance keys: " + ", ".join(missing))
    compiler = _trusted_compiler(properties)
    commands = _command_lines(build_metadata.compile_log_path)
    generated = (build_dir / "sketch" / f"{build_metadata.sketch_name}.ino.cpp").resolve()
    sketch_object = build_metadata.sketch_object_path.resolve()
    elf = build_metadata.application_elf_path.resolve()
    link_map = build_metadata.link_map_path.resolve()
    application = build_dir / build_metadata.application_binary_basename

    def matches_compile(tokens: list[str]) -> bool:
        return (
            len(tokens) >= 5 and "-c" in tokens and "-o" in tokens
            and tokens.count("-c") == 1 and tokens.count("-o") == 1
            and _path_token(tokens[0], build_dir) == compiler
            and _path_token(tokens[tokens.index("-o") + 1], build_dir) == sketch_object
            and _path_token(tokens[tokens.index("-o") - 1], build_dir) == generated
        )
    compile_commands = [tokens for tokens in commands if matches_compile(tokens)]
    if len(compile_commands) != 1:
        raise PackagingError("compile log must contain exactly one trusted generated-sketch compile command")
    if tuple(compile_commands[0]) != build_metadata.generated_compile_tokens:
        raise PackagingError(
            "compile log generated-sketch command differs from compile_commands.json"
        )

    def output_of(tokens: list[str]) -> Path | None:
        if tokens.count("-o") != 1 or tokens.index("-o") + 1 >= len(tokens):
            return None
        return _path_token(tokens[tokens.index("-o") + 1], build_dir)
    link_commands = [
        tokens for tokens in commands
        if tokens and _path_token(tokens[0], build_dir) == compiler and output_of(tokens) == elf
    ]
    if len(link_commands) != 1 or not any(token == "-Wl,--Map=" + link_map.as_posix() for token in link_commands[0]):
        raise PackagingError("compile log must contain exactly one trusted link command and map output")
    link_command = link_commands[0]
    pattern = properties["recipe.c.combine.pattern"]
    if "{object_files}" not in pattern or "{build.path}/{build.project_name}.elf" not in pattern:
        raise PackagingError("resolved linker recipe does not have the canonical Arduino placeholders")
    pattern_tokens = shlex.split(pattern, comments=False, posix=True)
    whole_archive = "-Wl,--whole-archive"
    no_whole_archive = "-Wl,--no-whole-archive"
    if recipe_whole_archive_controls(pattern_tokens):
        raise PackagingError("resolved linker recipe must not contain whole-archive controls")
    object_marker = pattern_tokens.index("{object_files}")
    if object_marker == 0 or pattern_tokens[object_marker - 1] != "-Wl,--start-group":
        raise PackagingError("resolved linker recipe must put object-files immediately after start-group")
    prefix = [token.replace("{build.path}", build_dir.as_posix()).replace("{build.project_name}", f"{build_metadata.sketch_name}.ino") for token in pattern_tokens[:object_marker]]
    if link_command[:len(prefix)] != prefix:
        raise PackagingError("verbose link command differs from resolved recipe.c.combine.pattern")
    suffix_template = pattern_tokens[object_marker + 1 :]
    if "{archive_file_path}" not in suffix_template:
        raise PackagingError("resolved linker recipe has no canonical core archive placeholder")
    archive_marker = suffix_template.index("{archive_file_path}")
    if archive_marker != 0:
        raise PackagingError("resolved linker recipe puts data before its core archive")
    expected_tail = [
        token.replace("{build.path}", build_dir.as_posix()).replace(
            "{build.project_name}", f"{build_metadata.sketch_name}.ino"
        )
        for token in suffix_template[archive_marker + 1 :]
    ]
    core_archive = (build_dir / "core" / "core.a").resolve()
    try:
        archive_index = link_command.index(core_archive.as_posix(), len(prefix))
    except ValueError as exc:
        raise PackagingError("verbose link command does not use the canonical core archive") from exc
    if link_command[archive_index + 1 :] != expected_tail:
        raise PackagingError("verbose link command suffix differs from resolved recipe.c.combine.pattern")
    # The Arduino recipe gives ``{object_files}`` one exact expansion window.
    # In a normal sketch it contains only regular build artifacts.  LVGL's
    # real expansion additionally encloses those artifacts in exactly one
    # whole/no-whole pair.  The controls are not recipe tokens, so accepting
    # them in the template would mask a substituted linker recipe.
    expanded_objects = link_command[len(prefix) : archive_index]
    if not expanded_objects:
        raise PackagingError("verbose link command has no object inputs")
    if expanded_objects[0] == whole_archive or expanded_objects[-1] == no_whole_archive:
        if (
            len(expanded_objects) < 3
            or expanded_objects[0] != whole_archive
            or expanded_objects[-1] != no_whole_archive
        ):
            raise PackagingError("expanded object-files whole-archive controls are not an exact balanced pair")
        object_tokens = expanded_objects[1:-1]
    else:
        object_tokens = expanded_objects
    if not object_tokens:
        raise PackagingError("verbose link command has no object inputs")
    if any(token.startswith("-Wl,") for token in object_tokens):
        raise PackagingError("expanded object-files contains an unexpected linker control")
    trusted_build = build_dir.resolve(strict=True)
    for token in object_tokens:
        candidate = Path(token)
        if not candidate.is_absolute():
            raise PackagingError("verbose link object input is not an absolute build artifact")
        if candidate.suffix not in {".o", ".a"} or candidate.is_symlink() or not candidate.is_file():
            raise PackagingError("verbose link object input is not a regular .o/.a artifact")
        try:
            candidate.resolve(strict=True).relative_to(trusted_build)
        except (FileNotFoundError, ValueError) as exc:
            raise PackagingError("verbose link object input escapes the build directory") from exc
    map_text = build_metadata.link_map_path.read_text(encoding="utf-8", errors="strict")
    object_reference = sketch_object.as_posix()
    archive_reference = re.compile(
        r"(?P<archive>[^\s()]+/sketch/objs\.a)\("
        + re.escape(f"{build_metadata.sketch_name}.ino.cpp.o")
        + r"\)"
    )
    if f"LOAD {object_reference}" not in map_text:
        archive_matches = list(archive_reference.finditer(map_text))
        if not archive_matches:
            raise PackagingError("link map does not bind the generated sketch object")
        archive_paths: set[Path] = set()
        for match in archive_matches:
            archive_candidate = Path(match.group("archive"))
            if (
                not archive_candidate.is_absolute()
                or archive_candidate.suffix != ".a"
                or archive_candidate.is_symlink()
                or not archive_candidate.is_file()
            ):
                raise PackagingError("link map sketch object archive is not a regular absolute .a artifact")
            try:
                archive_path = archive_candidate.resolve(strict=True)
                archive_path.relative_to(trusted_build)
            except (FileNotFoundError, ValueError) as exc:
                raise PackagingError("link map sketch object archive escapes the build directory") from exc
            archive_paths.add(archive_path)
        if len(archive_paths) != 1:
            raise PackagingError("link map sketch object members must resolve to one archive")
        archive_path = next(iter(archive_paths))
        member_name = f"{build_metadata.sketch_name}.ino.cpp.o"
        try:
            member = subprocess.run(
                ["ar", "p", str(archive_path), member_name],
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False, timeout=30,
            )
        except (OSError, subprocess.TimeoutExpired) as exc:
            raise PackagingError(f"cannot read link-map sketch archive member: {exc}") from exc
        if member.returncode or member.stdout != sketch_object.read_bytes():
            raise PackagingError("link-map sketch archive member differs from the independently compiled object")

    objcopy_args = shlex.split(properties["recipe.objcopy.bin.pattern_args"], comments=False, posix=True)
    if not objcopy_args or objcopy_args[0:2] != ["--chip", target] or "elf2image" not in objcopy_args:
        raise PackagingError("resolved objcopy recipe is not a canonical target elf2image command")
    expected_objcopy = [str(esptool), *objcopy_args]
    expected_objcopy = [
        token.replace("{build.path}", build_dir.as_posix()).replace("{build.project_name}", f"{build_metadata.sketch_name}.ino")
        for token in expected_objcopy
    ]
    if (
        expected_objcopy.count("-o") != 1
        or expected_objcopy.index("-o") + 1 >= len(expected_objcopy)
        or expected_objcopy[expected_objcopy.index("-o") + 1] != application.as_posix()
        or expected_objcopy.count(elf.as_posix()) != 1
        or expected_objcopy[-1] != elf.as_posix()
    ):
        raise PackagingError(
            "resolved objcopy recipe must use one application -o and final linked ELF input"
        )
    objcopy_commands = [tokens for tokens in commands if output_of(tokens) == application and "elf2image" in tokens]
    if len(objcopy_commands) != 1 or objcopy_commands[0] != expected_objcopy:
        raise PackagingError("verbose objcopy command differs from resolved recipe.objcopy.bin.pattern_args")

    _require_esptool_image_info(esptool, target, application, elf, repository_root)
    with tempfile.TemporaryDirectory(prefix=".arduino-replay-") as temporary:
        replay = Path(temporary)
        replay_object = replay / sketch_object.name
        replay_elf = replay / elf.name
        replay_map = replay / link_map.name
        replay_bin = replay / application.name
        _run_replay(
            _replace_output(compile_commands[0], sketch_object, replay_object),
            "generated sketch compile", repository_root,
        )
        if replay_object.read_bytes() != sketch_object.read_bytes():
            raise PackagingError("replayed generated sketch object differs from build output")
        replay_link = _replace_output(link_command, elf, replay_elf)
        replay_link = _replace_output(replay_link, link_map, replay_map)
        _run_replay(replay_link, "Arduino linker", repository_root)
        if replay_elf.read_bytes() != elf.read_bytes() or (
            canonical_link_map_bytes(link_map.read_bytes(), elf)
            != canonical_link_map_bytes(replay_map.read_bytes(), replay_elf)
        ):
            raise PackagingError("replayed Arduino link ELF/map differs from build output")
        replay_objcopy = _replace_output(expected_objcopy, application, replay_bin)
        replay_objcopy = _replace_output(replay_objcopy, elf, replay_elf)
        _run_replay(replay_objcopy, "Arduino objcopy", repository_root)
        if replay_bin.read_bytes() != application.read_bytes():
            raise PackagingError("replayed Arduino application binary differs from build output")


def load_board_metadata(
    properties_path: Path,
    layout: FlashLayout,
    build_dir: Path,
    build_metadata: BuildMetadata,
    target: str,
    requested_baud: int,
    requested_esptool_version: str,
    requested_framework_version: str,
    requested_fqbn: str,
    repository_root: Path,
) -> BoardMetadata:
    properties, raw = parse_properties(properties_path)
    required_properties = set(RESOLVED_PROPERTY_KEYS) | {
        "runtime.platform.path",
        "tools.esptool_py.path",
        "tools.esptool_py.cmd",
        "tools.esptool_py.upload.pattern_args",
        "runtime.tools.esp-rv32.path",
        "compiler.cpp.cmd",
        "recipe.c.combine.pattern",
        "recipe.objcopy.bin.pattern_args",
    }
    missing = sorted(required_properties - set(properties))
    if missing:
        raise PackagingError("resolved board properties are missing: " + ", ".join(missing))
    sanitized = {key: properties[key] for key in RESOLVED_PROPERTY_KEYS}
    if any(not SAFE_VALUE_RE.fullmatch(value) for value in sanitized.values()):
        raise PackagingError("resolved board properties contain an unsafe value")
    if sanitized["build.mcu"] != target:
        raise PackagingError("resolved build.mcu differs from compiled sdkconfig target")
    if sanitized["_id"] != requested_fqbn.split(":", 2)[2]:
        raise PackagingError("resolved board identity differs from requested FQBN")
    if sanitized["version"] != requested_framework_version:
        raise PackagingError("resolved Arduino core version differs from requested version")
    expected_flash = {
        "build.flash_mode": layout.settings["--flash-mode"],
        "build.flash_freq": layout.settings["--flash-freq"],
        "build.flash_size": layout.settings["--flash-size"],
    }
    for key, expected in expected_flash.items():
        if sanitized[key] != expected:
            raise PackagingError(f"resolved {key} differs from core-generated flash_args")
    if not sanitized["upload.speed"].isdigit():
        raise PackagingError("resolved upload.speed must be a decimal integer")
    if int(sanitized["upload.speed"]) != requested_baud:
        raise PackagingError("requested baud differs from resolved upload.speed")

    platform_path_text = properties["runtime.platform.path"]
    platform_input = Path(platform_path_text)
    if not platform_input.is_absolute() or platform_input.is_symlink():
        raise PackagingError("resolved Arduino platform path is not a canonical directory")
    try:
        platform_root = platform_input.resolve(strict=True)
    except FileNotFoundError as exc:
        raise PackagingError("resolved Arduino platform path does not exist") from exc
    if (
        not platform_root.is_dir()
        or platform_root.name != requested_framework_version
        or (platform_root / "platform.txt").is_symlink()
        or not (platform_root / "platform.txt").is_file()
    ):
        raise PackagingError(
            "resolved Arduino platform path does not match the requested core version"
        )

    upload_pattern = properties["tools.esptool_py.upload.pattern_args"]
    try:
        tokens = shlex.split(upload_pattern, comments=False, posix=True)
    except ValueError as exc:
        raise PackagingError(f"cannot parse resolved Arduino upload recipe: {exc}") from exc
    before_reset = recipe_value(tokens, "--before")
    after_reset = recipe_value(tokens, "--after")
    recipe_target = recipe_value(tokens, "--chip")
    recipe_baud = recipe_value(tokens, "--baud")
    if not recipe_baud.isdigit() or recipe_target != target or int(recipe_baud) != requested_baud:
        raise PackagingError("resolved upload recipe target/baud differs from build metadata")
    operations = [token for token in tokens if token in {"write-flash", "write_flash"}]
    if operations != ["write-flash"]:
        raise PackagingError("resolved upload recipe must use canonical esptool v5 write-flash")
    operation_index = tokens.index("write-flash")
    recipe_segments = parse_recipe_segments(
        tokens, operation_index, build_dir, platform_root, build_metadata, layout
    )
    if before_reset != "default-reset" or after_reset != "hard-reset":
        raise PackagingError(
            "resolved upload recipe reset modes are outside the validated helper contract"
        )

    tool_dir = Path(properties["tools.esptool_py.path"])
    tool_name = properties["tools.esptool_py.cmd"]
    if not SAFE_COMPONENT_RE.fullmatch(tool_name):
        raise PackagingError("resolved Arduino esptool command name is unsafe")
    esptool_binary = tool_dir / tool_name
    actual_version, binary_sha256 = read_esptool_version(esptool_binary)
    if actual_version != requested_esptool_version:
        raise PackagingError(
            f"resolved Arduino esptool is {actual_version}, expected {requested_esptool_version}"
        )
    verify_source_to_binary_provenance(
        properties, build_dir, build_metadata, target, esptool_binary, repository_root
    )
    return BoardMetadata(
        properties_evidence=source_evidence(properties_path, raw),
        properties_sha256=sha256_bytes(raw),
        sanitized_properties=sanitized,
        upload_pattern_sha256=sha256_bytes(upload_pattern.encode("utf-8")),
        before_reset=before_reset,
        after_reset=after_reset,
        operation="write-flash",
        recipe_segments=recipe_segments,
        esptool_binary_sha256=binary_sha256,
    )


def validate_git_sha(value: str, label: str = "git SHA") -> str:
    normalized = value.lower()
    if not GIT_SHA_RE.fullmatch(normalized):
        raise PackagingError(f"{label} must be a full 40-character hexadecimal SHA")
    return normalized


def validate_timestamp(value: str | None) -> str:
    if value is None:
        return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")
    if not value.endswith("Z"):
        raise PackagingError("timestamp must be an ISO-8601 UTC value ending in Z")
    try:
        parsed = datetime.fromisoformat(value[:-1] + "+00:00")
    except ValueError as exc:
        raise PackagingError(f"invalid timestamp: {value!r}") from exc
    if parsed.tzinfo != timezone.utc:
        raise PackagingError("timestamp must use UTC")
    return parsed.isoformat().replace("+00:00", "Z")


def expected_bsp_record() -> dict[str, object]:
    return {
        **EXPECTED_BSP_EVIDENCE,
        "registry_url": "https://components.espressif.com/",
        "used_by_build": False,
        "evidence_scope": (
            "task-level Registry verification; Arduino build does not link this BSP"
        ),
    }


def bsp_record_from_args(args: argparse.Namespace) -> dict[str, object]:
    values = {
        "component": args.bsp_component,
        "version": args.bsp_version,
        "component_hash": args.bsp_component_hash,
        "repository_commit": args.bsp_repository_commit,
    }
    present = {key for key, value in values.items() if value}
    if present != set(values):
        missing = sorted(set(values) - present)
        raise PackagingError("BSP evidence is incomplete; missing: " + ", ".join(missing))
    if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", values["component"]):
        raise PackagingError("invalid Registry BSP component name")
    if not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+(?:[-+][A-Za-z0-9.-]+)?", values["version"]):
        raise PackagingError("invalid Registry BSP version")
    component_hash = values["component_hash"].lower()
    if not SHA256_RE.fullmatch(component_hash):
        raise PackagingError("BSP component_hash must be a 64-character SHA-256 digest")
    repository_commit = validate_git_sha(values["repository_commit"], "BSP repository commit")
    supplied = {
        "component": values["component"],
        "version": values["version"],
        "component_hash": component_hash,
        "repository_commit": repository_commit,
    }
    if supplied != EXPECTED_BSP_EVIDENCE:
        raise PackagingError("BSP evidence does not match the task-verified Registry release")
    return expected_bsp_record()


def write_flash_arguments(
    target: str,
    baud: int,
    layout: FlashLayout,
    board: BoardMetadata,
    port_token: str,
) -> list[str]:
    arguments = [
        "python",
        "-m",
        "esptool",
        "--chip",
        target,
        "--port",
        port_token,
        "--baud",
        str(baud),
        "--before",
        board.before_reset,
        "--after",
        board.after_reset,
        board.operation,
    ]
    for flag in FLASH_SETTING_FLAGS:
        arguments.extend((flag, layout.settings[flag]))
    for segment in layout.segments:
        arguments.extend((f"{segment.offset:#x}", segment.archive_path))
    return arguments


def canonical_write_flash_command(
    target: str, baud: int, layout: FlashLayout, board: BoardMetadata
) -> str:
    arguments = write_flash_arguments(target, baud, layout, board, '"$PORT"')
    quoted: list[str] = []
    for token in arguments:
        if token == '"$PORT"':
            quoted.append(token)
        elif token.startswith("bin/"):
            quoted.append(f'"{token}"')
        else:
            quoted.append(token)
    return " ".join(quoted)


def shell_helper(
    target: str,
    baud: int,
    esptool_version: str,
    layout: FlashLayout,
    board: BoardMetadata,
) -> bytes:
    arguments = [
        "python", "-m", "esptool", "--chip", target, "--port", '"$FLASH_PORT"',
        "--baud", '"$FLASH_BAUD"', "--before", board.before_reset, "--after",
        board.after_reset, board.operation,
    ]
    for flag in FLASH_SETTING_FLAGS:
        arguments.extend((flag, layout.settings[flag]))
    lines = [
        "#!/usr/bin/env sh",
        "set -eu",
        f'REQUIRED_ESPTOOL_VERSION="{esptool_version}"',
        'ESPTOOL_VERSION_OUTPUT="$(python -m esptool version 2>&1)" || {',
        "  printf '%s\\n' \"Unable to run esptool==${REQUIRED_ESPTOOL_VERSION}.\" >&2",
        "  exit 2",
        "}",
        'printf \'%s\\n\' "$ESPTOOL_VERSION_OUTPUT" | grep -Fqx "$REQUIRED_ESPTOOL_VERSION" || {',
        "  printf '%s\\n' "
        '"Expected esptool==${REQUIRED_ESPTOOL_VERSION}; refusing to flash." >&2',
        "  exit 2",
        "}",
        'FLASH_PORT="${1:-${PORT:-}}"',
        '[ -n "$FLASH_PORT" ] || {',
        "  printf '%s\\n' 'Usage: ./flash.sh PORT [BAUD]' >&2",
        "  exit 2",
        "}",
        f'FLASH_BAUD="${{2:-${{BAUD:-{baud}}}}}"',
        " ".join(arguments) + " \\",
    ]
    segment_lines = []
    for index, segment in enumerate(layout.segments):
        suffix = " \\" if index + 1 < len(layout.segments) else ""
        segment_lines.append(f'  {segment.offset:#x} "{segment.archive_path}"{suffix}')
    return ("\n".join(lines + segment_lines) + "\n").encode("utf-8")


def windows_helper(
    target: str,
    baud: int,
    esptool_version: str,
    layout: FlashLayout,
    board: BoardMetadata,
) -> bytes:
    arguments = [
        "python", "-m", "esptool", "--chip", target, "--port", '"%FLASH_PORT%"',
        "--baud", '"%FLASH_BAUD%"', "--before", board.before_reset, "--after",
        board.after_reset, board.operation,
    ]
    for flag in FLASH_SETTING_FLAGS:
        arguments.extend((flag, layout.settings[flag]))
    for segment in layout.segments:
        windows_path = segment.archive_path.replace("/", chr(92))
        arguments.extend((f"{segment.offset:#x}", f'"{windows_path}"'))
    lines = [
        "@echo off",
        "setlocal",
        f'set "REQUIRED_ESPTOOL_VERSION={esptool_version}"',
        'set "ESPTOOL_VERSION_OK="',
        "for /f \"delims=\" %%V in ('python -m esptool version 2^>^&1') do (",
        '  if "%%V"=="%REQUIRED_ESPTOOL_VERSION%" set "ESPTOOL_VERSION_OK=1"',
        ")",
        "if not defined ESPTOOL_VERSION_OK (",
        "  echo Expected esptool==%REQUIRED_ESPTOOL_VERSION%; refusing to flash.",
        "  exit /b 2",
        ")",
        'set "FLASH_PORT=%~1"',
        'if not defined FLASH_PORT set "FLASH_PORT=%PORT%"',
        "if not defined FLASH_PORT (",
        "  echo Usage: flash.cmd PORT [BAUD]",
        "  exit /b 2",
        ")",
        'set "FLASH_BAUD=%~2"',
        'if not defined FLASH_BAUD set "FLASH_BAUD=%BAUD%"',
        f'if not defined FLASH_BAUD set "FLASH_BAUD={baud}"',
        " ".join(arguments),
        "if errorlevel 1 exit /b %errorlevel%",
        "endlocal",
    ]
    return ("\r\n".join(lines) + "\r\n").encode("utf-8")


def packaged_flash_args(layout: FlashLayout) -> bytes:
    lines = [
        " ".join(f"{flag} {layout.settings[flag]}" for flag in FLASH_SETTING_FLAGS)
    ]
    lines.extend(
        f"{segment.offset:#x} {segment.archive_path}" for segment in layout.segments
    )
    return ("\n".join(lines) + "\n").encode("utf-8")


def slugify(value: str) -> str:
    slug = re.sub(r"[^A-Za-z0-9]+", "-", value).strip("-").lower()
    if not slug:
        raise PackagingError(f"cannot derive artifact name from {value!r}")
    return slug


def checksum_document(members: dict[str, bytes]) -> bytes:
    if CHECKSUMS_SELF_PATH in members:
        raise PackagingError("checksum document must not include itself as input")
    lines = [f"{sha256_bytes(members[name])}  {name}" for name in sorted(members)]
    return ("\n".join(lines) + "\n").encode("utf-8")


def zip_member_mode(name: str) -> int:
    return 0o755 if name == "flash.sh" else 0o644


def write_zip(path: Path, members: dict[str, bytes]) -> None:
    names = [validate_archive_name(name) for name in members]
    if len(names) != len(set(names)):
        raise PackagingError("duplicate ZIP member path")
    with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name in sorted(members):
            info = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
            info.create_system = 3
            info.create_version = 20
            info.extract_version = 20
            info.flag_bits = 0
            info.volume = 0
            info.internal_attr = 0
            info.external_attr = (stat.S_IFREG | zip_member_mode(name)) << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, members[name])


def parse_checksums(data: bytes) -> dict[str, str]:
    try:
        lines = data.decode("utf-8").splitlines()
    except UnicodeDecodeError as exc:
        raise PackagingError("SHA256SUMS.txt is not valid UTF-8") from exc
    checksums: dict[str, str] = {}
    for line in lines:
        match = CHECKSUM_LINE_RE.fullmatch(line)
        if not match:
            raise PackagingError(f"invalid SHA256SUMS line: {line!r}")
        name = validate_archive_name(match.group(2))
        if name == CHECKSUMS_SELF_PATH or name in checksums:
            raise PackagingError(f"invalid checksum member boundary: {name}")
        checksums[name] = match.group(1)
    if not checksums:
        raise PackagingError("SHA256SUMS.txt is empty")
    return checksums


def validate_public_text_members(contents: dict[str, bytes]) -> None:
    """Reject host/user paths and cache/workspace details from public text."""

    for name in sorted(PUBLIC_TEXT_MEMBERS & set(contents)):
        try:
            text = contents[name].decode("utf-8")
        except UnicodeDecodeError as exc:
            raise PackagingError(f"public text member is not UTF-8: {name}") from exc
        scan_text = text.replace("#!/usr/bin/env sh", "")
        for pattern in PRIVATE_TEXT_PATTERNS:
            match = pattern.search(scan_text)
            if match:
                raise PackagingError(
                    f"public text member exposes a private host path/token: {name}"
                )
        for token in PRIVATE_USER_TOKENS:
            if re.search(
                rf"(?<![A-Za-z0-9._-]){re.escape(token)}(?![A-Za-z0-9._-])",
                scan_text,
                re.IGNORECASE,
            ):
                raise PackagingError(
                    f"public text member exposes a private username token: {name}"
                )


def validate_binary_path_candidate(run: bytes, match: re.Match[bytes]) -> None:
    """Allow only the enumerated, observed stock diagnostic paths."""

    candidate = match.group(0)
    # These are exact complete printable runs observed in deterministic output,
    # not prefixes or path patterns. Any surrounding byte changes the run and
    # therefore falls through to the normal fail-closed path checks below.
    if run in OBSERVED_RANDOM_BINARY_PATH_RUNS or run in STOCK_EXACT_BINARY_PATH_RUNS:
        return
    start, end = match.span()
    previous = run[start - 1 : start] if start else b""
    previous_two = run[start - 2 : start] if start >= 2 else b""
    compiler_absolute_operand = bool(
        COMPILER_ABSOLUTE_PATH_PREFIX_RE.search(run[:start])
    )
    if candidate[1:4] == b"://" and re.fullmatch(
        rb"[A-Za-z][A-Za-z0-9+.-]*", run[: start + 1]
    ):
        return
    # A slash inside a safe relative label or ordinary diagnostic word is not
    # an absolute path.  Compiler-style absolute operands remain candidates.
    if (
        start
        and candidate.startswith(b"/")
        and not candidate.startswith(b"//")
        and previous in b"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._-"
        and previous_two not in {b"-I", b"-L"}
        and not compiler_absolute_operand
    ):
        return
    # URLs are public network identifiers rather than local filesystem paths.
    # A single-slash ``foo:/usr/...`` value is still rejected below.
    if previous == b":" and candidate.startswith(b"//"):
        scheme_prefix = run[: start - 1]
        if re.search(rb"(?:^|[^A-Za-z0-9+.-])[A-Za-z][A-Za-z0-9+.-]*$", scheme_prefix):
            return
    if (
        previous in {b"@", b":"}
        or previous_two in {b"-I", b"-L"}
        or compiler_absolute_operand
        or b"@" in candidate
        or (b"//" in candidate[1:] and not candidate.startswith(b"//IDF/"))
        or (
            candidate.startswith(b"//")
            and previous not in {b"", b" ", b'"', b"'", b"(", b"[", b"{"}
        )
        or (end < len(run) and run[end : end + 1] in {b"/", b"\\", b"?", b":"})
    ):
        raise PackagingError("public artifact has a malformed or prefixed absolute path")
    if candidate.startswith(b"/"):
        if STOCK_SYNTHETIC_PATH_RE.fullmatch(candidate):
            if candidate.startswith(b"/./managed_components/"):
                synthetic_tail = candidate.removeprefix(b"/./managed_components/")
            else:
                synthetic_tail = candidate.removeprefix(b"/product/")
            if any(
                component in {b"", b".", b".."}
                for component in synthetic_tail.split(b"/")
            ):
                raise PackagingError("public artifact has a non-canonical synthetic path")
            return
        stripped = candidate[2:] if candidate.startswith(b"//") else candidate[1:]
        components = stripped.split(b"/")
        if any(component in {b"", b".", b".."} for component in components):
            raise PackagingError("public artifact has a non-canonical absolute path")
    if not STOCK_BINARY_PATH_RE.fullmatch(candidate):
        raise PackagingError("public artifact has a non-allowlisted absolute path")


def validate_public_artifact_privacy(contents: dict[str, bytes]) -> None:
    """Reject host paths/user identities from every public member, including BINs."""

    validate_public_text_members(contents)
    for name, data in sorted(contents.items()):
        if name in PUBLIC_TEXT_MEMBERS:
            continue
        # Treat printable runs independently.  Never join across NULs: doing so
        # fabricates paths that do not exist in the public binary.
        for run_match in PRINTABLE_RUN_RE.finditer(data):
            scan_data = run_match.group(0)
            for path_match in ABSOLUTE_PATH_CANDIDATE_RE.finditer(scan_data):
                try:
                    validate_binary_path_candidate(scan_data, path_match)
                except PackagingError as exc:
                    raise PackagingError(
                        f"public artifact member exposes a private host path/token: {name}"
                    ) from exc
            privacy_scan_data = PUBLIC_NETWORK_URL_RE.sub(b"", scan_data)
            if any(pattern.search(privacy_scan_data) for pattern in PRIVATE_BINARY_PATTERNS):
                raise PackagingError(
                    f"public artifact member exposes a private host path/token: {name}"
                )
            for token in PRIVATE_USER_TOKENS:
                encoded = re.escape(token.encode("utf-8"))
                if re.search(
                    rb"(?<![A-Za-z0-9._-])" + encoded + rb"(?![A-Za-z0-9._-])",
                    privacy_scan_data,
                    re.IGNORECASE,
                ):
                    raise PackagingError(
                        f"public artifact member exposes a private username token: {name}"
                    )


def validate_zip_container(path: Path) -> None:
    """Require a contiguous canonical local-record chain and single-disk ZIP."""

    raw = path.read_bytes()
    if not raw.startswith(ZIP_LOCAL_FILE_HEADER):
        raise PackagingError("ZIP must begin with a canonical local-file header")
    eocd_index = raw.rfind(ZIP_END_OF_CENTRAL_DIRECTORY)
    if eocd_index < 0 or eocd_index + ZIP_EOCD.size > len(raw):
        raise PackagingError("ZIP end-of-central-directory record is missing")
    (
        signature,
        disk_number,
        central_disk,
        disk_entries,
        total_entries,
        central_size,
        central_offset,
        comment_length,
    ) = ZIP_EOCD.unpack_from(raw, eocd_index)
    if signature != ZIP_END_OF_CENTRAL_DIRECTORY:
        raise PackagingError("ZIP end-of-central-directory signature is invalid")
    if (
        disk_number != 0
        or central_disk != 0
        or disk_entries != total_entries
        or total_entries <= 0
    ):
        raise PackagingError("ZIP must use one complete non-empty disk")
    if central_offset + central_size != eocd_index:
        raise PackagingError("ZIP central-directory boundary is non-canonical")
    if comment_length != 0 or eocd_index + ZIP_EOCD.size != len(raw):
        raise PackagingError("ZIP comments or trailing bytes are forbidden")

    try:
        with zipfile.ZipFile(path) as archive:
            central_infos = archive.infolist()
            if [info.filename for info in central_infos] != sorted(
                info.filename for info in central_infos
            ):
                raise PackagingError("ZIP central member order is not canonical sorted order")
            infos = sorted(central_infos, key=lambda info: info.header_offset)
    except (OSError, zipfile.BadZipFile) as exc:
        raise PackagingError(f"cannot parse ZIP local-file records: {exc}") from exc
    if len(infos) != total_entries:
        raise PackagingError("ZIP local/central entry counts differ")

    cursor = 0
    local_names: list[str] = []
    for info in infos:
        validate_archive_name(info.filename)
        if info.header_offset != cursor or cursor + ZIP_LOCAL_HEADER.size > central_offset:
            raise PackagingError(
                "ZIP local-file records must form one canonical contiguous chain"
            )
        (
            local_signature,
            version_needed,
            flags,
            compression,
            dos_time,
            dos_date,
            crc,
            compressed_size,
            uncompressed_size,
            filename_length,
            extra_length,
        ) = ZIP_LOCAL_HEADER.unpack_from(raw, cursor)
        if local_signature != ZIP_LOCAL_FILE_HEADER:
            raise PackagingError("ZIP local-file header signature is invalid")
        name_start = cursor + ZIP_LOCAL_HEADER.size
        name_end = name_start + filename_length
        data_start = name_end + extra_length
        data_end = data_start + compressed_size
        if data_end > central_offset:
            raise PackagingError("ZIP local-file record exceeds the local-data boundary")
        try:
            expected_name = info.filename.encode("ascii")
        except UnicodeEncodeError as exc:
            raise PackagingError("ZIP member names must use canonical ASCII") from exc
        if raw[name_start:name_end] != expected_name:
            raise PackagingError("ZIP local and central member names differ")
        local_names.append(info.filename)
        if extra_length != 0:
            raise PackagingError("ZIP local-file extra fields are forbidden")
        if (
            version_needed != 20
            or
            flags != 0
            or compression != zipfile.ZIP_DEFLATED
            or dos_time != 0
            or dos_date != 33
            or crc != info.CRC
            or compressed_size != info.compress_size
            or uncompressed_size != info.file_size
        ):
            raise PackagingError("ZIP local-file metadata is non-canonical")
        compressed = raw[data_start:data_end]
        inflater = zlib.decompressobj(-15)
        try:
            decompressed = inflater.decompress(compressed) + inflater.flush()
        except zlib.error as exc:
            raise PackagingError("ZIP DEFLATE payload cannot be decoded") from exc
        if (
            not inflater.eof
            or inflater.unused_data
            or inflater.unconsumed_tail
            or len(decompressed) != uncompressed_size
            or (zlib.crc32(decompressed) & 0xFFFFFFFF) != crc
        ):
            raise PackagingError("ZIP DEFLATE payload has non-canonical trailing or checksum data")
        cursor = data_end
    if cursor != central_offset:
        raise PackagingError(
            "ZIP local-file records do not exactly reach the central directory"
        )
    if local_names != sorted(local_names):
        raise PackagingError("ZIP local member order is not canonical sorted order")


def validated_evidence_record(value: object, label: str) -> SourceEvidence:
    if not isinstance(value, dict) or set(value) != {"basename", "size", "sha256"}:
        raise PackagingError(f"{label} evidence has an invalid schema")
    basename = value.get("basename")
    size = value.get("size")
    digest = value.get("sha256")
    if not isinstance(basename, str) or not SAFE_COMPONENT_RE.fullmatch(basename):
        raise PackagingError(f"{label} evidence basename is unsafe")
    if not isinstance(size, int) or isinstance(size, bool) or size <= 0:
        raise PackagingError(f"{label} evidence size must be a positive integer")
    if not isinstance(digest, str) or not SHA256_RE.fullmatch(digest):
        raise PackagingError(f"{label} evidence SHA-256 is invalid")
    return SourceEvidence(basename=basename, size=size, sha256=digest)


def validate_archive(
    path: Path,
    *,
    local_build: LocalBuildEvidence | None,
    expected_manifest: bytes | None = None,
) -> dict[str, object]:
    """Reopen a package and verify paths, hashes, bytes, and offset provenance."""

    trusted = derive_trusted_repository()
    if local_build is None:
        raise PackagingError(
            "actual local build evidence is required for source-to-binary provenance validation"
        )
    validate_zip_container(path)
    try:
        with zipfile.ZipFile(path) as archive:
            if archive.comment:
                raise PackagingError("ZIP archive comment is forbidden")
            infos = archive.infolist()
            names: list[str] = []
            for info in infos:
                name = validate_archive_name(info.filename)
                mode = info.external_attr >> 16
                if stat.S_ISLNK(mode):
                    raise PackagingError(f"ZIP member must not be a symlink: {name}")
                if info.is_dir():
                    raise PackagingError(f"ZIP contains an unexpected directory entry: {name}")
                if info.extra:
                    raise PackagingError(f"ZIP member extra fields are forbidden: {name}")
                if info.comment:
                    raise PackagingError(f"ZIP member comments are forbidden: {name}")
                expected_mode = stat.S_IFREG | zip_member_mode(name)
                if (
                    info.create_system != 3
                    or info.create_version != 20
                    or info.extract_version != 20
                    or info.flag_bits != 0
                    or info.volume != 0
                    or getattr(info, "reserved", 0) != 0
                    or info.internal_attr != 0
                    or mode != expected_mode
                    or info.external_attr != expected_mode << 16
                ):
                    raise PackagingError(f"ZIP member mode is non-canonical: {name}")
                if (
                    info.compress_type != zipfile.ZIP_DEFLATED
                    or info.date_time != (1980, 1, 1, 0, 0, 0)
                ):
                    raise PackagingError(f"ZIP member encoding is non-canonical: {name}")
                names.append(name)
            if len(names) != len(set(names)):
                raise PackagingError("ZIP contains duplicate member names")
            missing = sorted(REQUIRED_ARCHIVE_MEMBERS - set(names))
            if missing:
                raise PackagingError("ZIP is missing required members: " + ", ".join(missing))
            contents = {name: archive.read(name) for name in names}
    except (OSError, zipfile.BadZipFile) as exc:
        raise PackagingError(f"cannot read package ZIP: {exc}") from exc

    validate_public_artifact_privacy(contents)
    checksums = parse_checksums(contents[CHECKSUMS_SELF_PATH])
    signed_members = set(contents) - {CHECKSUMS_SELF_PATH}
    if set(checksums) != signed_members:
        raise PackagingError(
            "SHA256SUMS checksum coverage boundary mismatch: "
            f"missing={sorted(signed_members - set(checksums))} "
            f"extra={sorted(set(checksums) - signed_members)}"
        )
    for name, expected in checksums.items():
        actual = sha256_bytes(contents[name])
        if actual != expected:
            raise PackagingError(f"ZIP member checksum mismatch: {name}")

    manifest_bytes = contents["manifest.json"]
    if expected_manifest is not None and manifest_bytes != expected_manifest:
        raise PackagingError("standalone and packaged manifests differ")
    try:
        manifest = json.loads(manifest_bytes.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise PackagingError(f"cannot parse packaged manifest: {exc}") from exc
    if (
        not isinstance(manifest, dict)
        or type(manifest.get("schema_version")) is not int
        or manifest.get("schema_version") != 1
    ):
        raise PackagingError("unsupported Arduino package manifest schema")
    if set(manifest) != MANIFEST_KEYS:
        raise PackagingError("Arduino package manifest has an invalid top-level schema")
    if manifest.get("artifact_type") != EXPECTED_ARTIFACT_TYPE:
        raise PackagingError("Arduino package manifest artifact_type is invalid")
    project_path_value = manifest.get("project_path")
    name_value = manifest.get("name")
    if not isinstance(project_path_value, str):
        raise PackagingError("manifest project_path is invalid")
    project_path = PurePosixPath(validate_archive_name(project_path_value))
    if (
        not isinstance(name_value, str)
        or not SAFE_COMPONENT_RE.fullmatch(name_value)
        or project_path.name != name_value
    ):
        raise PackagingError("manifest name does not match its repository-relative project_path")
    product_git_sha = manifest.get("product_git_sha")
    if not isinstance(product_git_sha, str) or product_git_sha != validate_git_sha(
        product_git_sha, "manifest product_git_sha"
    ):
        raise PackagingError("manifest product_git_sha must be lowercase full 40-hex")
    if product_git_sha != trusted.head:
        raise PackagingError(
            "manifest product_git_sha does not match the validator's trusted repository HEAD"
        )
    timestamp = manifest.get("timestamp_utc")
    if not isinstance(timestamp, str) or validate_timestamp(timestamp) != timestamp:
        raise PackagingError("manifest timestamp_utc is not canonical UTC")
    bsp = manifest.get("bsp")
    if (
        not isinstance(bsp, dict)
        or type(bsp.get("used_by_build")) is not bool
        or bsp.get("used_by_build") is not False
        or bsp != expected_bsp_record()
    ):
        raise PackagingError("manifest BSP evidence does not match the exact verified tuple")
    integrity = manifest.get("integrity")
    if integrity != {
        "algorithm": "sha256",
        "checksums_file": CHECKSUMS_SELF_PATH,
        "checksums_excludes": [CHECKSUMS_SELF_PATH],
    }:
        raise PackagingError("manifest checksum boundary does not match package policy")

    framework = manifest.get("framework")
    if (
        not isinstance(framework, dict)
        or set(framework) != FRAMEWORK_KEYS
        or framework.get("name") != "Arduino-ESP32"
    ):
        raise PackagingError("manifest framework identity is invalid")
    for key in ("version", "arduino_cli_version"):
        value = framework.get(key)
        if not isinstance(value, str) or not SAFE_VALUE_RE.fullmatch(value):
            raise PackagingError(f"manifest framework {key} is invalid")

    build = manifest.get("build")
    if not isinstance(build, dict) or set(build) != BUILD_KEYS:
        raise PackagingError("manifest build identity must be an object")
    if build.get("identity_metadata_path") != "metadata/build_identity.json":
        raise PackagingError("manifest build identity metadata path is invalid")
    try:
        identity_document = json.loads(contents["metadata/build_identity.json"].decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise PackagingError(f"cannot parse packaged build identity: {exc}") from exc
    if (
        not isinstance(identity_document, dict)
        or type(identity_document.get("schema_version")) is not int
        or identity_document.get("schema_version") != 1
        or set(identity_document) != {"schema_version", "build"}
        or not isinstance(identity_document.get("build"), dict)
    ):
        raise PackagingError("packaged build identity has an invalid schema")
    expected_identity = {
        "schema_version": 1,
        "build": {
            key: value for key, value in build.items() if key != "identity_metadata_path"
        },
    }
    if identity_document != expected_identity:
        raise PackagingError("packaged build identity differs from manifest")

    raw_evidence = build.get("raw_evidence")
    if not isinstance(raw_evidence, dict) or set(raw_evidence) != set(RAW_EVIDENCE_KEYS):
        raise PackagingError("manifest raw build evidence has an invalid schema")
    evidence_records = {
        key: validated_evidence_record(raw_evidence[key], key)
        for key in RAW_EVIDENCE_KEYS
    }
    options_evidence = evidence_records["build_options"]
    properties_evidence = evidence_records["board_properties"]
    if options_evidence.basename != "build.options.json":
        raise PackagingError("build options evidence basename is not canonical")
    expected_evidence_basenames = {
        "compile_commands": "compile_commands.json",
        "primary_source": f"{name_value}.ino",
        "generated_translation_unit": f"{name_value}.ino.cpp",
        "sketch_object": f"{name_value}.ino.cpp.o",
        "application_elf": f"{name_value}.ino.elf",
        "link_map": f"{name_value}.ino.map",
    }
    for key, basename in expected_evidence_basenames.items():
        if evidence_records[key].basename != basename:
            raise PackagingError(f"{key} evidence basename is not canonical")
    for key, evidence in evidence_records.items():
        if build.get(f"{key}_sha256") != evidence.sha256:
            raise PackagingError(f"{key} evidence SHA differs from manifest")
    trusted_sketch_dir, _trusted_sketch_path = resolve_repository_sketch(
        trusted.root, project_path.as_posix()
    )
    trusted_primary_source = trusted_sketch_dir / f"{name_value}.ino"
    if (
        trusted_primary_source.is_symlink()
        or not trusted_primary_source.is_file()
        or source_evidence(trusted_primary_source) != evidence_records["primary_source"]
    ):
        raise PackagingError(
            "manifest primary source evidence differs from the validator's trusted repository"
        )
    if local_build.build_dir.is_symlink():
        raise PackagingError("actual local Arduino build directory must not be a symlink")
    try:
        local_build_dir = local_build.build_dir.resolve(strict=True)
    except FileNotFoundError as exc:
        raise PackagingError("actual local Arduino build directory is missing") from exc
    if not local_build_dir.is_dir():
        raise PackagingError("actual local Arduino build path is not a directory")
    local_paths = {
        "build_options": local_build_dir / "build.options.json",
        "board_properties": local_build.board_properties,
        "compile_commands": local_build_dir / "compile_commands.json",
        "primary_source": trusted_primary_source,
        "generated_translation_unit": (
            local_build_dir / "sketch" / f"{name_value}.ino.cpp"
        ),
        "sketch_object": local_build_dir / "sketch" / f"{name_value}.ino.cpp.o",
        "application_elf": local_build_dir / f"{name_value}.ino.elf",
        "link_map": local_build_dir / f"{name_value}.ino.map",
        "compile_log": local_build.compile_log,
    }
    for key, evidence_path in local_paths.items():
        if source_evidence(evidence_path) != evidence_records[key]:
            raise PackagingError(
                f"packaged {key} evidence differs from the actual local build/source file"
            )

    canonical_build_identity = {
        "sketch_name": name_value,
        "primary_source_basename": f"{name_value}.ino",
        "generated_translation_unit_basename": f"{name_value}.ino.cpp",
        "application_binary_basename": f"{name_value}.ino.bin",
    }
    for key, expected in canonical_build_identity.items():
        if build.get(key) != expected:
            raise PackagingError(f"manifest canonical build identity is invalid: {key}")
    application_binary_basename = canonical_build_identity["application_binary_basename"]

    source_line_count = build.get("source_line_count")
    if (
        not isinstance(source_line_count, int)
        or isinstance(source_line_count, bool)
        or source_line_count <= 0
    ):
        raise PackagingError("manifest source_line_count is invalid")
    source_line_mapping_sha256 = build.get("source_line_mapping_sha256")
    compile_identity_sha256 = build.get("compile_identity_sha256")
    if not isinstance(source_line_mapping_sha256, str) or not SHA256_RE.fullmatch(
        source_line_mapping_sha256
    ):
        raise PackagingError("manifest source_line_mapping_sha256 is invalid")
    if not isinstance(compile_identity_sha256, str) or not SHA256_RE.fullmatch(
        compile_identity_sha256
    ):
        raise PackagingError("manifest compile_identity_sha256 is invalid")
    primary_path = local_paths["primary_source"]
    generated_path = local_paths["generated_translation_unit"]
    try:
        generated_text = generated_path.read_text(encoding="utf-8")
    except UnicodeDecodeError as exc:
        raise PackagingError("local generated translation unit is not UTF-8") from exc
    recomputed = compute_source_line_mapping(generated_text, primary_path)
    if recomputed != (
        source_line_count,
        source_line_mapping_sha256,
        compile_identity_sha256,
    ):
        raise PackagingError(
            "manifest compile identity differs from the actual local generated "
            "translation unit and primary source"
        )

    fqbn = build.get("fqbn")
    resolved_fqbn = build.get("resolved_fqbn")
    board_options = build.get("board_options")
    target = build.get("target")
    if not isinstance(fqbn, str) or not FQBN_RE.fullmatch(fqbn):
        raise PackagingError("manifest FQBN is invalid")
    if not isinstance(resolved_fqbn, str):
        raise PackagingError("manifest resolved FQBN is invalid")
    resolved_parts = resolved_fqbn.split(":", 3)
    if len(resolved_parts) < 3 or ":".join(resolved_parts[:3]) != fqbn:
        raise PackagingError("manifest resolved FQBN differs from requested FQBN")
    resolved_option_text = resolved_parts[3] if len(resolved_parts) == 4 else ""
    if not isinstance(board_options, dict) or board_options != parse_board_options(
        resolved_option_text
    ):
        raise PackagingError("manifest board options differ from resolved FQBN")
    if not isinstance(target, str) or not TARGET_RE.fullmatch(target):
        raise PackagingError("manifest build target is invalid")
    resolved_properties = build.get("resolved_properties")
    if not isinstance(resolved_properties, dict) or set(resolved_properties) != set(
        RESOLVED_PROPERTY_KEYS
    ):
        raise PackagingError("manifest resolved board properties have an invalid schema")
    if any(
        not isinstance(value, str) or not SAFE_VALUE_RE.fullmatch(value)
        for value in resolved_properties.values()
    ):
        raise PackagingError("manifest resolved board properties contain unsafe values")
    if resolved_properties["build.mcu"] != target:
        raise PackagingError("manifest target differs from resolved build.mcu")
    if resolved_properties["_id"] != fqbn.split(":", 2)[2]:
        raise PackagingError("manifest resolved board identity differs from FQBN")
    if resolved_properties["version"] != framework["version"]:
        raise PackagingError("manifest resolved core version differs from framework version")

    toolchain = manifest.get("toolchain")
    if not isinstance(toolchain, dict) or set(toolchain) != {"esptool"}:
        raise PackagingError("manifest toolchain evidence has an invalid schema")
    esptool = toolchain.get("esptool")
    if not isinstance(esptool, dict) or set(esptool) != {
        "version",
        "requirement",
        "bundled_binary_sha256",
    }:
        raise PackagingError("manifest esptool evidence has an invalid schema")
    if esptool.get("version") != EXPECTED_ESPTOOL_VERSION or esptool.get(
        "requirement"
    ) != f"esptool=={EXPECTED_ESPTOOL_VERSION}":
        raise PackagingError("manifest must pin the validated bundled esptool version")
    binary_sha = esptool.get("bundled_binary_sha256")
    if not isinstance(binary_sha, str) or not SHA256_RE.fullmatch(binary_sha):
        raise PackagingError("manifest bundled esptool SHA-256 is invalid")
    if contents["requirements.txt"] != f"esptool=={EXPECTED_ESPTOOL_VERSION}\n".encode():
        raise PackagingError("packaged requirements must pin exact esptool version")

    flash = manifest.get("flash")
    if not isinstance(flash, dict) or set(flash) != FLASH_KEYS:
        raise PackagingError("manifest flash record must be an object")
    raw_bytes = contents["metadata/flash_args.source.txt"]
    provenance = flash.get("offset_provenance", {})
    if provenance != {
        "kind": "arduino_core_generated_flash_args",
        "archive_path": "metadata/flash_args.source.txt",
        "sha256": sha256_bytes(raw_bytes),
    }:
        raise PackagingError("manifest offset provenance does not match raw flash_args")
    try:
        raw_layout = parse_flash_args_text(raw_bytes.decode("utf-8"))
        packaged_layout = parse_flash_args_text(contents["flash_args"].decode("utf-8"))
    except UnicodeDecodeError as exc:
        raise PackagingError("packaged flash arguments are not valid UTF-8") from exc

    manifest_settings = flash.get("settings")
    expected_settings = {
        flag.removeprefix("--").replace("-", "_"): raw_layout.settings[flag]
        for flag in FLASH_SETTING_FLAGS
    }
    if manifest_settings != expected_settings:
        raise PackagingError("manifest flash settings differ from raw flash_args")
    capacity = parse_flash_size(raw_layout.settings["--flash-size"])
    if type(flash.get("capacity_bytes")) is not int or flash.get("capacity_bytes") != capacity:
        raise PackagingError("manifest flash capacity differs from raw flash_args")
    property_flash_settings = {
        "build.flash_mode": raw_layout.settings["--flash-mode"],
        "build.flash_freq": raw_layout.settings["--flash-freq"],
        "build.flash_size": raw_layout.settings["--flash-size"],
    }
    for key, expected in property_flash_settings.items():
        if resolved_properties[key] != expected:
            raise PackagingError(f"resolved {key} differs from raw flash_args")
    baud = flash.get("baud")
    if (
        not isinstance(baud, int)
        or isinstance(baud, bool)
        or baud <= 0
        or resolved_properties["upload.speed"] != str(baud)
    ):
        raise PackagingError("manifest baud differs from resolved upload.speed")

    manifest_segments = flash.get("segments")
    if not isinstance(manifest_segments, list):
        raise PackagingError("manifest flash segments must be a list")
    expected_segment_members: set[str] = set()
    offsets: set[int] = set()
    source_names: set[str] = set()
    verified_segments: list[FlashSegment] = []
    if len(manifest_segments) != len(raw_layout.segments):
        raise PackagingError("manifest segment count differs from raw flash_args")
    for item, (offset, source_name) in zip(manifest_segments, raw_layout.segments):
        if not isinstance(item, dict) or any(
            type(item.get(key)) is not int for key in ("offset", "size", "end")
        ):
            raise PackagingError("manifest segment numeric fields must be exact integers")
        archive_name = f"bin/{source_name}"
        expected_segment_members.add(archive_name)
        data = contents.get(archive_name)
        if data is None:
            raise PackagingError(f"ZIP is missing flash segment: {archive_name}")
        expected_item = {
            "offset": offset,
            "offset_hex": f"{offset:#x}",
            "source_name": source_name,
            "path": archive_name,
            "role": segment_role(source_name, application_binary_basename),
            "size": len(data),
            "end": offset + len(data),
            "end_hex": f"{offset + len(data):#x}",
            "sha256": sha256_bytes(data),
        }
        if item != expected_item:
            raise PackagingError(
                "manifest segment differs from raw flash_args or ZIP bytes: "
                f"{archive_name}"
            )
        if offset in offsets or source_name in source_names:
            raise PackagingError("packaged flash segments have duplicate offsets or files")
        if not data:
            raise PackagingError(f"packaged flash segment is empty: {archive_name}")
        if offset + len(data) > capacity:
            raise PackagingError(f"packaged flash segment exceeds capacity: {archive_name}")
        lowered = PurePosixPath(source_name).name.lower()
        if (
            "merged" in lowered
            or "whole-flash" in lowered
            or "whole_flash" in lowered
            or (offset == 0 and len(data) == capacity)
        ):
            raise PackagingError(f"packaged merged/whole-flash image is forbidden: {archive_name}")
        offsets.add(offset)
        source_names.add(source_name)
        verified_segments.append(
            FlashSegment(
                offset=offset,
                source_name=source_name,
                source_path=Path(archive_name),
                archive_path=archive_name,
                role=segment_role(source_name, application_binary_basename),
                size=len(data),
                sha256=sha256_bytes(data),
            )
        )

    ordered_segments = sorted(verified_segments, key=lambda segment: segment.offset)
    for previous, current in zip(ordered_segments, ordered_segments[1:]):
        if current.offset < previous.end:
            raise PackagingError(
                f"packaged flash segments overlap: {previous.archive_path} and "
                f"{current.archive_path}"
            )
    required_roles = {"bootloader", "partition_table", "application"}
    actual_roles = {segment.role for segment in verified_segments}
    if not required_roles.issubset(actual_roles):
        raise PackagingError(
            "packaged image is not a complete segmented Arduino image; missing roles: "
            + ", ".join(sorted(required_roles - actual_roles))
        )
    payload_total = sum(segment.size for segment in verified_segments)
    if payload_total >= capacity:
        raise PackagingError(
            "packaged segmented payload total must be strictly smaller than flash capacity"
        )
    if (
        type(flash.get("segmented_payload_total")) is not int
        or flash.get("segmented_payload_total") != payload_total
    ):
        raise PackagingError("manifest segmented_payload_total differs from ZIP bytes")

    packaged_expected = tuple(
        (offset, f"bin/{source_name}") for offset, source_name in raw_layout.segments
    )
    if (
        packaged_layout.settings != raw_layout.settings
        or packaged_layout.segments != packaged_expected
    ):
        raise PackagingError("packaged flash_args differs from raw core flash_args")

    recipe = flash.get("recipe_provenance")
    if not isinstance(recipe, dict):
        raise PackagingError("manifest upload recipe provenance is missing")
    upload_pattern_sha = recipe.get("upload_pattern_sha256")
    if not isinstance(upload_pattern_sha, str) or not SHA256_RE.fullmatch(upload_pattern_sha):
        raise PackagingError("manifest upload recipe SHA-256 is invalid")
    if not isinstance(recipe.get("offsets"), list) or any(
        type(offset) is not int for offset in recipe["offsets"]
    ) or not isinstance(recipe.get("segments"), list) or any(
        not isinstance(item, dict) or type(item.get("offset")) is not int
        for item in recipe["segments"]
    ):
        raise PackagingError("manifest upload recipe numeric fields must be exact integers")
    expected_recipe = {
        "kind": "arduino_cli_board_details_expanded_upload_recipe",
        "board_properties_sha256": properties_evidence.sha256,
        "upload_pattern_sha256": upload_pattern_sha,
        "before_reset": "default-reset",
        "after_reset": "hard-reset",
        "operation": "write-flash",
        "offsets": [segment.offset for segment in verified_segments],
        "segments": [
            {"offset": segment.offset, "source_name": segment.source_name}
            for segment in verified_segments
        ],
    }
    if recipe != expected_recipe:
        raise PackagingError("manifest upload recipe differs from segmented flash evidence")

    reconstructed_layout = FlashLayout(
        settings=dict(raw_layout.settings),
        capacity_bytes=capacity,
        raw_bytes=raw_bytes,
        raw_sha256=sha256_bytes(raw_bytes),
        segments=tuple(verified_segments),
    )
    reconstructed_board = BoardMetadata(
        properties_evidence=properties_evidence,
        properties_sha256=properties_evidence.sha256,
        sanitized_properties=dict(resolved_properties),
        upload_pattern_sha256=upload_pattern_sha,
        before_reset="default-reset",
        after_reset="hard-reset",
        operation="write-flash",
        recipe_segments=tuple(
            (segment.offset, segment.source_name) for segment in verified_segments
        ),
        esptool_binary_sha256=binary_sha,
    )
    expected_command = canonical_write_flash_command(
        target, baud, reconstructed_layout, reconstructed_board
    )
    if flash.get("write_flash_command") != expected_command:
        raise PackagingError("manifest write_flash_command differs from verified segments")

    allowed_members = REQUIRED_ARCHIVE_MEMBERS | expected_segment_members
    if set(contents) != allowed_members:
        raise PackagingError(
            "ZIP member set differs from segmented package contract: "
            f"extra={sorted(set(contents) - allowed_members)} "
            f"missing={sorted(allowed_members - set(contents))}"
        )
    expected_helpers = {
        "flash.sh": shell_helper(
            target,
            baud,
            EXPECTED_ESPTOOL_VERSION,
            reconstructed_layout,
            reconstructed_board,
        ),
        "flash.cmd": windows_helper(
            target,
            baud,
            EXPECTED_ESPTOOL_VERSION,
            reconstructed_layout,
            reconstructed_board,
        ),
    }
    for helper_name, expected_helper in expected_helpers.items():
        if contents[helper_name] != expected_helper:
            raise PackagingError(f"flash helper differs from verified v5 recipe: {helper_name}")

    local_build_metadata = load_build_metadata(
        local_build_dir,
        trusted.root,
        project_path.as_posix(),
        fqbn,
        resolved_option_text,
        local_build.compile_log,
    )
    expected_local_build = {
        "resolved_fqbn": resolved_fqbn,
        "board_options": board_options,
        "target": target,
        "sketch_name": name_value,
        "primary_source_basename": f"{name_value}.ino",
        "generated_translation_unit_basename": f"{name_value}.ino.cpp",
        "application_binary_basename": application_binary_basename,
        "source_line_count": source_line_count,
        "source_line_mapping_sha256": source_line_mapping_sha256,
        "compile_identity_sha256": compile_identity_sha256,
    }
    actual_local_build = {
        key: getattr(local_build_metadata, key) for key in expected_local_build
    }
    if actual_local_build != expected_local_build:
        raise PackagingError(
            "manifest build identity differs from the actual local Arduino build"
        )

    local_layout = load_flash_layout(local_build_dir, application_binary_basename)
    if (
        local_layout.settings != raw_layout.settings
        or local_layout.capacity_bytes != capacity
        or local_layout.raw_bytes != raw_bytes
        or len(local_layout.segments) != len(verified_segments)
    ):
        raise PackagingError(
            "packaged flash layout differs from the actual local Arduino build"
        )
    for local_segment, packaged_segment in zip(local_layout.segments, verified_segments):
        if (
            local_segment.offset != packaged_segment.offset
            or local_segment.source_name != packaged_segment.source_name
            or local_segment.role != packaged_segment.role
            or local_segment.size != packaged_segment.size
            or local_segment.sha256 != packaged_segment.sha256
            or local_segment.source_path.read_bytes()
            != contents[packaged_segment.archive_path]
        ):
            raise PackagingError(
                "packaged segment bytes differ from the actual local Arduino build"
            )

    local_board = load_board_metadata(
        local_build.board_properties,
        local_layout,
        local_build_dir,
        local_build_metadata,
        target,
        baud,
        EXPECTED_ESPTOOL_VERSION,
        framework["version"],
        fqbn,
        trusted.root,
    )
    if (
        local_board.properties_evidence != properties_evidence
        or local_board.sanitized_properties != resolved_properties
        or local_board.upload_pattern_sha256 != upload_pattern_sha
        or local_board.before_reset != "default-reset"
        or local_board.after_reset != "hard-reset"
        or local_board.operation != "write-flash"
        or local_board.recipe_segments
        != tuple((segment.offset, segment.source_name) for segment in verified_segments)
        or local_board.esptool_binary_sha256 != binary_sha
    ):
        raise PackagingError(
            "manifest board/tool recipe differs from the actual local Arduino build metadata"
        )
    return manifest


def build_manifest(
    *,
    args: argparse.Namespace,
    layout: FlashLayout,
    build_metadata: BuildMetadata,
    board: BoardMetadata,
    git_sha: str,
    timestamp: str,
    bsp: dict[str, object],
) -> dict[str, object]:
    sketch_path = PurePosixPath(validate_archive_name(args.sketch_path))
    return {
        "schema_version": 1,
        "artifact_type": EXPECTED_ARTIFACT_TYPE,
        "name": sketch_path.name,
        "project_path": sketch_path.as_posix(),
        "product_git_sha": git_sha,
        "timestamp_utc": timestamp,
        "framework": {
            "name": "Arduino-ESP32",
            "version": args.framework_version,
            "arduino_cli_version": args.arduino_cli_version,
        },
        "build": {
            "fqbn": args.fqbn,
            "resolved_fqbn": build_metadata.resolved_fqbn,
            "board_options": dict(sorted(build_metadata.board_options.items())),
            "target": build_metadata.target,
            "sketch_name": build_metadata.sketch_name,
            "primary_source_basename": build_metadata.primary_source_basename,
            "generated_translation_unit_basename": (
                build_metadata.generated_translation_unit_basename
            ),
            "application_binary_basename": build_metadata.application_binary_basename,
            "identity_metadata_path": "metadata/build_identity.json",
            "build_options_sha256": build_metadata.options_evidence.sha256,
            "board_properties_sha256": board.properties_sha256,
            "compile_commands_sha256": build_metadata.compile_commands_evidence.sha256,
            "primary_source_sha256": build_metadata.primary_source_evidence.sha256,
            "generated_translation_unit_sha256": (
                build_metadata.generated_translation_unit_evidence.sha256
            ),
            "sketch_object_sha256": build_metadata.sketch_object_evidence.sha256,
            "application_elf_sha256": build_metadata.application_elf_evidence.sha256,
            "link_map_sha256": build_metadata.link_map_evidence.sha256,
            "compile_log_sha256": build_metadata.compile_log_evidence.sha256,
            "source_line_count": build_metadata.source_line_count,
            "source_line_mapping_sha256": build_metadata.source_line_mapping_sha256,
            "compile_identity_sha256": build_metadata.compile_identity_sha256,
            "resolved_properties": board.sanitized_properties,
            "raw_evidence": {
                "build_options": evidence_document(build_metadata.options_evidence),
                "board_properties": evidence_document(board.properties_evidence),
                "compile_commands": evidence_document(
                    build_metadata.compile_commands_evidence
                ),
                "primary_source": evidence_document(
                    build_metadata.primary_source_evidence
                ),
                "generated_translation_unit": evidence_document(
                    build_metadata.generated_translation_unit_evidence
                ),
                "sketch_object": evidence_document(build_metadata.sketch_object_evidence),
                "application_elf": evidence_document(build_metadata.application_elf_evidence),
                "link_map": evidence_document(build_metadata.link_map_evidence),
                "compile_log": evidence_document(build_metadata.compile_log_evidence),
            },
        },
        "toolchain": {
            "esptool": {
                "version": args.esptool_version,
                "requirement": f"esptool=={args.esptool_version}",
                "bundled_binary_sha256": board.esptool_binary_sha256,
            }
        },
        "bsp": bsp,
        "flash": {
            "baud": args.baud,
            "capacity_bytes": layout.capacity_bytes,
            "segmented_payload_total": sum(segment.size for segment in layout.segments),
            "settings": {
                flag.removeprefix("--").replace("-", "_"): layout.settings[flag]
                for flag in FLASH_SETTING_FLAGS
            },
            "offset_provenance": {
                "kind": "arduino_core_generated_flash_args",
                "archive_path": "metadata/flash_args.source.txt",
                "sha256": layout.raw_sha256,
            },
            "recipe_provenance": {
                "kind": "arduino_cli_board_details_expanded_upload_recipe",
                "board_properties_sha256": board.properties_sha256,
                "upload_pattern_sha256": board.upload_pattern_sha256,
                "before_reset": board.before_reset,
                "after_reset": board.after_reset,
                "operation": board.operation,
                "offsets": [offset for offset, _source_name in board.recipe_segments],
                "segments": [
                    {"offset": offset, "source_name": source_name}
                    for offset, source_name in board.recipe_segments
                ],
            },
            "write_flash_command": canonical_write_flash_command(
                build_metadata.target, args.baud, layout, board
            ),
            "segments": [
                {
                    "offset": segment.offset,
                    "offset_hex": f"{segment.offset:#x}",
                    "source_name": segment.source_name,
                    "path": segment.archive_path,
                    "role": segment.role,
                    "size": segment.size,
                    "end": segment.end,
                    "end_hex": f"{segment.end:#x}",
                    "sha256": segment.sha256,
                }
                for segment in layout.segments
            ],
        },
        "integrity": {
            "algorithm": "sha256",
            "checksums_file": CHECKSUMS_SELF_PATH,
            "checksums_excludes": [CHECKSUMS_SELF_PATH],
        },
    }


def package(args: argparse.Namespace) -> tuple[Path, Path]:
    build_dir = args.build_dir.resolve()
    trusted = derive_trusted_repository()
    repository_root = trusted.root
    output_dir = args.output_dir.resolve()
    if output_dir.exists():
        raise PackagingError(f"output directory already exists: {output_dir}")
    if args.esptool_version != EXPECTED_ESPTOOL_VERSION:
        raise PackagingError(
            f"this workflow requires esptool=={EXPECTED_ESPTOOL_VERSION}"
        )
    git_sha = validate_git_sha(args.git_sha)
    if git_sha != trusted.head:
        raise PackagingError(
            "product git SHA does not match the packager's own repository HEAD"
        )
    build_metadata = load_build_metadata(
        build_dir,
        repository_root,
        args.sketch_path,
        args.fqbn,
        args.board_options,
        args.compile_log,
    )
    layout = load_flash_layout(
        build_dir, build_metadata.application_binary_basename
    )
    board = load_board_metadata(
        args.board_properties,
        layout,
        build_dir,
        build_metadata,
        build_metadata.target,
        args.baud,
        args.esptool_version,
        args.framework_version,
        args.fqbn,
        repository_root,
    )
    timestamp = validate_timestamp(args.timestamp_utc)
    bsp = bsp_record_from_args(args)
    manifest = build_manifest(
        args=args,
        layout=layout,
        build_metadata=build_metadata,
        board=board,
        git_sha=git_sha,
        timestamp=timestamp,
        bsp=bsp,
    )
    manifest_bytes = (json.dumps(manifest, indent=2, sort_keys=True) + "\n").encode("utf-8")
    build_identity_bytes = (
        json.dumps(
            {
                "schema_version": 1,
                "build": {
                    key: value
                    for key, value in manifest["build"].items()
                    if key != "identity_metadata_path"
                },
            },
            indent=2,
            sort_keys=True,
        )
        + "\n"
    ).encode("utf-8")
    members: dict[str, bytes] = {
        "manifest.json": manifest_bytes,
        "metadata/build_identity.json": build_identity_bytes,
        "metadata/flash_args.source.txt": layout.raw_bytes,
        "flash_args": packaged_flash_args(layout),
        "requirements.txt": f"esptool=={args.esptool_version}\n".encode("utf-8"),
        "flash.sh": shell_helper(
            build_metadata.target, args.baud, args.esptool_version, layout, board
        ),
        "flash.cmd": windows_helper(
            build_metadata.target, args.baud, args.esptool_version, layout, board
        ),
    }
    for segment in layout.segments:
        data = segment.source_path.read_bytes()
        if len(data) != segment.size or sha256_bytes(data) != segment.sha256:
            raise PackagingError(f"build segment changed while packaging: {segment.source_name}")
        members[segment.archive_path] = data
    validate_public_artifact_privacy(members)
    members[CHECKSUMS_SELF_PATH] = checksum_document(members)

    archive_name = (
        f"arduino-{slugify(PurePosixPath(args.sketch_path).name)}-"
        f"{slugify(args.framework_version)}-{build_metadata.target}-{git_sha[:12]}.zip"
    )
    output_dir.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".arduino-package-", dir=output_dir.parent) as temp:
        stage = Path(temp)
        staged_manifest = stage / "manifest.json"
        staged_archive = stage / archive_name
        staged_manifest.write_bytes(manifest_bytes)
        write_zip(staged_archive, members)
        local_build_evidence = LocalBuildEvidence(
            build_dir=build_dir,
            board_properties=args.board_properties,
            compile_log=args.compile_log,
        )
        validate_archive(
            staged_archive,
            local_build=local_build_evidence,
            expected_manifest=manifest_bytes,
        )
        output_dir.mkdir()
        os.replace(staged_manifest, output_dir / "manifest.json")
        os.replace(staged_archive, output_dir / archive_name)
    outputs = sorted(path.name for path in output_dir.iterdir())
    if outputs != sorted(("manifest.json", archive_name)):
        raise PackagingError(f"unexpected package outputs: {outputs!r}")
    return output_dir / archive_name, output_dir / "manifest.json"


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--board-properties", type=Path, required=True)
    parser.add_argument("--compile-log", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--sketch-path", required=True)
    parser.add_argument("--framework-version", required=True)
    parser.add_argument("--arduino-cli-version", required=True)
    parser.add_argument("--esptool-version", required=True)
    parser.add_argument("--fqbn", required=True)
    parser.add_argument("--board-options", required=True)
    parser.add_argument("--git-sha", required=True)
    parser.add_argument("--timestamp-utc")
    parser.add_argument("--baud", type=int, required=True)
    parser.add_argument("--bsp-component", required=True)
    parser.add_argument("--bsp-version", required=True)
    parser.add_argument("--bsp-component-hash", required=True)
    parser.add_argument("--bsp-repository-commit", required=True)
    args = parser.parse_args(argv)
    if args.baud <= 0:
        parser.error("--baud must be positive")
    return args


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    try:
        archive, manifest = package(args)
    except (PackagingError, OSError) as exc:
        print(f"error: {exc}", file=os.sys.stderr)
        return 2
    print(json.dumps({"archive": archive.as_posix(), "manifest": manifest.as_posix()}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
