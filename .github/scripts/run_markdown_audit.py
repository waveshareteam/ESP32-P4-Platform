#!/usr/bin/env python3
"""Run the repository-local Markdown audit with fail-closed CI scope handling."""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

from ci_change_routing import ChangeListError, is_documentation_path, read_name_status


def github_output(name: str, value: str) -> None:
    output_path = os.environ.get("GITHUB_OUTPUT")
    if output_path:
        with open(output_path, "a", encoding="utf-8") as output:
            output.write(f"{name}={value}\n")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", type=Path, required=True)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--changed-files-from", type=Path)
    source.add_argument("--all", action="store_true")
    parser.add_argument("--config", type=Path, required=True)
    parser.add_argument("--format", choices=("text", "json"), default="json")
    parser.add_argument("--strict", action="store_true")
    args = parser.parse_args(argv)

    command = [
        sys.executable,
        str(Path(__file__).with_name("audit_markdown.py")),
        str(args.repo),
        "--config",
        str(args.config),
        "--format",
        args.format,
    ]
    if args.all:
        github_output("docs_only", "false")
        command.append("--all")
    else:
        try:
            changes = read_name_status(args.changed_files_from)
        except ChangeListError as exc:
            print(f"Could not read changed-file evidence: {exc}", file=sys.stderr)
            return 2
        docs_only = all(is_documentation_path(change.path) for change in changes)
        github_output("docs_only", "true" if docs_only else "false")
        command.extend(("--changed-files-from", str(args.changed_files_from)))
        if docs_only:
            command.append("--expect-docs-only")

    if args.strict:
        command.append("--strict")

    return subprocess.run(command, check=False).returncode


if __name__ == "__main__":
    raise SystemExit(main())
