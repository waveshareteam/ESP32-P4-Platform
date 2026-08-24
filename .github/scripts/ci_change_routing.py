#!/usr/bin/env python3
"""Shared changed-file parsing and routing policy for example CI."""

from __future__ import annotations

import fnmatch
from dataclasses import dataclass
from pathlib import Path, PurePosixPath


class ChangeListError(ValueError):
    """Raised when changed-file evidence is missing or malformed."""


@dataclass(frozen=True)
class Change:
    """One path participating in a Git name-status change."""

    status: str
    path: str
    deleted: bool = False


@dataclass(frozen=True)
class RoutingResult:
    """Framework-specific routing decision plus review evidence."""

    selected: tuple[str, ...]
    docs_only: bool
    firmware_paths: tuple[str, ...]
    unknown_paths: tuple[str, ...]
    removed_entries: tuple[str, ...]
    changed_path_count: int


MARKDOWN_SUFFIXES = {".md", ".mdx"}
DOCUMENTATION_ASSET_SUFFIXES = {".gif", ".jpeg", ".jpg", ".png", ".svg", ".webp"}
DOCUMENTATION_ROOT_FILES = {
    "CHANGELOG.md",
    "CODE_OF_CONDUCT.md",
    "CONTRIBUTING.md",
    "CONTRIBUTING_CN.md",
    "LICENSE",
    "LICENSE.md",
    "LICENSE.txt",
    "README.md",
    "README_CN.md",
    "SECURITY.md",
    "SECURITY_CN.md",
    "SUPPORT.md",
    "SUPPORT_CN.md",
    "THIRD_PARTY.md",
}
POLICY_NON_BUILD_PATHS = {
    ".github/workflows/repository-policy.yml",
    ".github/scripts/audit_markdown.py",
    ".github/scripts/run_markdown_audit.py",
    ".github/scripts/tests/test_markdown_audit.py",
    "config/markdown-audit.json",
}


def normalize_path(value: str) -> str:
    """Normalize Git paths without resolving them on the host filesystem."""

    return value.replace("\\", "/").strip().strip("/")


def parse_name_status(lines: list[str]) -> list[Change]:
    """Parse `git diff --name-status` output, retaining both rename paths."""

    changes: list[Change] = []
    for line_number, raw_line in enumerate(lines, start=1):
        line = raw_line.rstrip("\r\n")
        if not line:
            continue

        fields = line.split("\t")
        status = fields[0]
        code = status[:1]
        if code in {"R", "C"}:
            if len(fields) != 3:
                raise ChangeListError(
                    f"line {line_number}: {status} must contain old and new paths"
                )
            old_path = normalize_path(fields[1])
            new_path = normalize_path(fields[2])
            if not old_path or not new_path:
                raise ChangeListError(f"line {line_number}: empty rename/copy path")
            changes.append(Change(status=status, path=old_path, deleted=code == "R"))
            changes.append(Change(status=status, path=new_path))
            continue

        if code not in {"A", "D", "M", "T", "U", "X", "B"} or len(fields) != 2:
            raise ChangeListError(
                f"line {line_number}: expected STATUS<TAB>PATH, got {raw_line!r}"
            )
        path = normalize_path(fields[1])
        if not path:
            raise ChangeListError(f"line {line_number}: empty changed path")
        changes.append(Change(status=status, path=path, deleted=code == "D"))

    if not changes:
        raise ChangeListError("changed-file input is empty")
    return changes


def read_name_status(path: Path) -> list[Change]:
    """Read a UTF-8 name-status file and fail closed when it is unavailable."""

    if not path.is_file():
        raise ChangeListError(f"changed-file input does not exist: {path}")
    try:
        return parse_name_status(path.read_text(encoding="utf-8").splitlines())
    except UnicodeDecodeError as exc:
        raise ChangeListError(f"changed-file input is not valid UTF-8: {path}") from exc


def is_documentation_path(path: str) -> bool:
    """Return whether a path is documentation/governance with no build impact."""

    normalized = normalize_path(path)
    pure = PurePosixPath(normalized)
    suffix = pure.suffix.lower()
    if suffix in MARKDOWN_SUFFIXES:
        return True
    if normalized in DOCUMENTATION_ROOT_FILES:
        return True
    if normalized == ".github/PULL_REQUEST_TEMPLATE.md":
        return True
    if normalized.startswith(".github/ISSUE_TEMPLATE/"):
        return True
    if suffix in DOCUMENTATION_ASSET_SUFFIXES:
        return (
            normalized.startswith("assets/")
            or normalized.startswith("docs/")
            or "/pic/" in normalized
        )
    return False


def is_policy_non_build_path(path: str) -> bool:
    """Return whether a policy-only path should skip example build selection."""

    return normalize_path(path) in POLICY_NON_BUILD_PATHS


def matches_any(path: str, patterns: tuple[str, ...]) -> bool:
    return any(fnmatch.fnmatch(path, pattern) for pattern in patterns)


def entry_for_path(path: str, root: PurePosixPath) -> str | None:
    """Map a changed path to the direct child project/sketch below root."""

    normalized = normalize_path(path)
    root_text = root.as_posix()
    if normalized == root_text:
        return root_text
    if not normalized.startswith(root_text + "/"):
        return None
    parts = PurePosixPath(normalized).parts
    if len(parts) <= len(root.parts):
        return root_text
    return PurePosixPath(*parts[: len(root.parts) + 1]).as_posix()


def route_changes(
    changes: list[Change],
    *,
    known_entries: set[str],
    root: PurePosixPath,
    other_framework_root: PurePosixPath,
    global_patterns: tuple[str, ...],
    other_framework_patterns: tuple[str, ...] = (),
) -> RoutingResult:
    """Select affected entries while reporting non-build and unknown scopes."""

    if not changes:
        raise ChangeListError("changed-file evidence is empty")

    selected: set[str] = set()
    firmware_paths: set[str] = set()
    unknown_paths: set[str] = set()
    removed_entries: set[str] = set()
    build_all = False
    unique_paths = {change.path for change in changes}
    docs_only = all(is_documentation_path(path) for path in unique_paths)
    root_text = root.as_posix()
    other_root_text = other_framework_root.as_posix()

    for change in changes:
        path = change.path
        if path.startswith("firmware/"):
            firmware_paths.add(path)
            continue
        if is_policy_non_build_path(path):
            continue
        if is_documentation_path(path):
            continue
        if matches_any(path, other_framework_patterns):
            continue
        if path == other_root_text or path.startswith(other_root_text + "/"):
            continue
        if matches_any(path, global_patterns):
            build_all = True
            continue

        entry = entry_for_path(path, root)
        if entry is not None:
            if entry == root_text:
                build_all = True
            elif entry in known_entries:
                selected.add(entry)
            elif change.deleted:
                removed_entries.add(entry)
                # The removed project cannot be built, but surviving projects may
                # still reference its shared files or override paths.
                build_all = True
            else:
                # A new or structurally incomplete entry must not silently pass.
                unknown_paths.add(path)
                build_all = True
            continue

        # A complete but unfamiliar non-document path is conservatively global.
        unknown_paths.add(path)
        build_all = True

    if build_all:
        selected.update(known_entries)

    return RoutingResult(
        selected=tuple(sorted(selected)),
        docs_only=docs_only,
        firmware_paths=tuple(sorted(firmware_paths)),
        unknown_paths=tuple(sorted(unknown_paths)),
        removed_entries=tuple(sorted(removed_entries)),
        changed_path_count=len(unique_paths),
    )
