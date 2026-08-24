#!/usr/bin/env python3
"""Synthetic acceptance tests for the repository-local Markdown policy gate."""

from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[3]
SCRIPTS_ROOT = REPO_ROOT / ".github" / "scripts"
sys.path.insert(0, str(SCRIPTS_ROOT))

import audit_markdown  # noqa: E402


AUDIT = SCRIPTS_ROOT / "audit_markdown.py"
WRAPPER = SCRIPTS_ROOT / "run_markdown_audit.py"
CONFIG = REPO_ROOT / "config" / "markdown-audit.json"


def write_files(root: Path, files: dict[str, str]) -> None:
    for relative, content in files.items():
        destination = root / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text(content, encoding="utf-8")


def paired_config(
    root: Path, *, homepage: bool = False, guide_pair: bool = False
) -> Path:
    config: dict[str, object] = {
        "bilingual_pairs": [{"english": "README.md", "chinese": "README_CN.md"}],
        "homepage_pairs": [],
    }
    if homepage:
        config["homepage_pairs"] = [
            {
                "english": "README.md",
                "chinese": "README_CN.md",
                "profile": "multi-product-hub",
                "required_components": ["badges", "quick_links", "hero_image"],
                "required_quick_links": ["documentation"],
                "required_badges": ["build"],
            }
        ]
    if guide_pair:
        config["bilingual_pairs"].append(
            {"english": "GUIDE.md", "chinese": "GUIDE_CN.md"}
        )
    path = root / "audit.json"
    path.write_text(json.dumps(config), encoding="utf-8")
    return path


def changed_file(root: Path, lines: list[str]) -> Path:
    path = root / "changed.txt"
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return path


def run(command: list[str], *, env: dict[str, str] | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=env,
        encoding="utf-8",
        errors="replace",
    )


class ConfigurationAndOwnershipTests(unittest.TestCase):
    def test_complete_repository_config_parses(self) -> None:
        config = audit_markdown.load_config(CONFIG)
        self.assertEqual(config["homepage_pairs"][0]["profile"], "multi-product-hub")

    def test_ownership_rules_preserve_wrappers_and_embedded_upstream(self) -> None:
        config = audit_markdown.load_config(CONFIG)
        self.assertEqual(
            audit_markdown.classify(
                "examples/esp-idf/09_sdmmc/components/sd_card/README.md", config
            ),
            "first_party_wrapper",
        )
        self.assertEqual(
            audit_markdown.classify(
                "examples/esp-idf/18_esp_brookesia_phone/components/brookesia_core/README.md",
                config,
            ),
            "embedded_upstream",
        )
        self.assertEqual(
            audit_markdown.classify(
                "examples/esp-idf/18_esp_brookesia_phone/components/brookesia_app_squareline_demo/README.md",
                config,
            ),
            "first_party_wrapper",
        )


class AuditRuleTests(unittest.TestCase):
    def test_reciprocal_bilingual_and_language_routing(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            config = paired_config(root, guide_pair=True)
            write_files(
                root,
                {
                    "README.md": "[中文](README_CN.md)\n# English\n[Guide](GUIDE.md)\n",
                    "README_CN.md": "[English](README.md)\n# 中文\n[指南](GUIDE_CN.md)\n",
                "GUIDE.md": "[中文](GUIDE_CN.md)\n# Guide\n",
                "GUIDE_CN.md": "[English](GUIDE.md)\n# 指南\n",
                },
            )
            changed = changed_file(root, ["M\tREADME.md", "M\tREADME_CN.md"])
            result = run(
                [sys.executable, str(AUDIT), str(root), "--changed-files-from", str(changed), "--config", str(config), "--strict", "--format", "json"]
            )
            self.assertEqual(result.returncode, 0, result.stderr + result.stdout)

            write_files(root, {"README_CN.md": "[English](README.md)\n# 中文\n[Guide](GUIDE.md)\n"})
            result = run(
                [sys.executable, str(AUDIT), str(root), "--changed-files-from", str(changed), "--config", str(config), "--format", "json"]
            )
            self.assertEqual(result.returncode, 1)
            self.assertIn("WRONG_LANGUAGE_INTERNAL_LINK", result.stdout)

    def test_unconfigured_cn_suffix_fails_strict(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            config = paired_config(root)
            write_files(
                root,
                {
                    "README.md": "[中文](README_CN.md)\n# English\n",
                    "README_CN.md": "[English](README.md)\n# 中文\n",
                    "NOTES.md": "[简体中文](NOTES_CN.md)\n# Notes\n",
                    "NOTES_CN.md": "[English](NOTES.md)\n# 说明\n",
                },
            )
            changed = changed_file(root, ["M\tNOTES.md", "M\tNOTES_CN.md"])
            result = run(
                [sys.executable, str(AUDIT), str(root), "--changed-files-from", str(changed), "--config", str(config), "--strict", "--format", "json"]
            )
            self.assertEqual(result.returncode, 1)
            self.assertIn("NONSTANDARD_FIRST_PARTY_LANGUAGE_SUFFIX", result.stdout)

    def test_markdown_reference_and_fragment_links(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            config = paired_config(root)
            pages = {
                "README.md": "[中文](README_CN.md)\n# English\n[Guide][guide]\n[Fragment](GUIDE.md#target)\n[guide]: GUIDE.md\n",
                "README_CN.md": "[English](README.md)\n# 中文\n[指南](GUIDE_CN.md)\n",
                "GUIDE.md": "# Target\n",
                "GUIDE_CN.md": "# 目标\n",
            }
            write_files(root, pages)
            changed = changed_file(root, ["M\tREADME.md", "M\tREADME_CN.md"])
            result = run([sys.executable, str(AUDIT), str(root), "--changed-files-from", str(changed), "--config", str(config), "--format", "json"])
            self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
            write_files(root, {"README.md": pages["README.md"].replace("#target", "#missing")})
            result = run([sys.executable, str(AUDIT), str(root), "--changed-files-from", str(changed), "--config", str(config), "--format", "json"])
            self.assertEqual(result.returncode, 1)
            self.assertIn("RELATIVE_LINK_FRAGMENT_MISSING", result.stdout)

    def test_homepage_structure_and_symmetry(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            config = paired_config(root, homepage=True)
            catalog_image = '<a href="https://example.com/product"><img alt="Product" src="https://example.com/product.png"></a>\n'
            header = "<div align=\"center\">\n<h1>Platform</h1>\n<strong>Subtitle</strong>\n" + catalog_image + "<a href=\"https://example.com/workflow\"><img alt=\"Build\" src=\"https://example.com/badge.svg\"></a>\n<a href=\"README_CN.md\">中文</a> · <a href=\"https://example.com/docs\">📚 Documentation</a>\n</div>\n\n---\n\n## ✨ Overview\n"
            write_files(root, {"README.md": header, "README_CN.md": header.replace("中文", "English").replace("README_CN.md", "README.md")})
            changed = changed_file(root, ["M\tREADME.md", "M\tREADME_CN.md"])
            result = run([sys.executable, str(AUDIT), str(root), "--changed-files-from", str(changed), "--config", str(config), "--format", "json"])
            self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
            write_files(root, {"README_CN.md": (root / "README_CN.md").read_text(encoding="utf-8").replace(catalog_image, "")})
            result = run([sys.executable, str(AUDIT), str(root), "--changed-files-from", str(changed), "--config", str(config), "--format", "json"])
            self.assertEqual(result.returncode, 1)
            self.assertIn("HOMEPAGE_REQUIRED_COMPONENT_MISSING", result.stdout)
            write_files(root, {"README_CN.md": header.replace("中文", "English").replace("README_CN.md", "README.md")})
            write_files(root, {"README_CN.md": (root / "README_CN.md").read_text(encoding="utf-8").replace("## ✨ Overview", "## 📚 Overview")})
            result = run([sys.executable, str(AUDIT), str(root), "--changed-files-from", str(changed), "--config", str(config), "--format", "json"])
            self.assertEqual(result.returncode, 1)
            self.assertIn("HOMEPAGE_H2", result.stdout)

    def test_privacy_and_provenance_are_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            config = paired_config(root)
            write_files(root, {"README.md": "[中文](README_CN.md)\n# Guide\nWritten by Codex at C:\\Users\\person\\repo\n", "README_CN.md": "[English](README.md)\n# 指南\n"})
            changed = changed_file(root, ["M\tREADME.md", "M\tREADME_CN.md"])
            result = run([sys.executable, str(AUDIT), str(root), "--changed-files-from", str(changed), "--config", str(config), "--format", "json"])
            self.assertEqual(result.returncode, 1)
            self.assertIn("LOCAL_ABSOLUTE_PATH", result.stdout)
            self.assertIn("TOOL_OR_MODEL_PROVENANCE", result.stdout)

    def test_docs_only_rejects_source_config_and_delivery_artifacts(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            config = paired_config(root)
            write_files(root, {"README.md": "[中文](README_CN.md)\n# Guide\n", "README_CN.md": "[English](README.md)\n# 指南\n", "main.c": "", "settings.json": "{}", "firmware.bin": "", "delivery.zip": ""})
            changed = changed_file(root, ["M\tREADME.md", "M\tREADME_CN.md", "M\tmain.c", "M\tsettings.json", "M\tfirmware.bin", "M\tdelivery.zip"])
            result = run([sys.executable, str(AUDIT), str(root), "--changed-files-from", str(changed), "--config", str(config), "--expect-docs-only", "--format", "json"])
            self.assertEqual(result.returncode, 1)
            for code in ("DOCS_ONLY_SOURCE_CHANGE", "DOCS_ONLY_CONFIG_CHANGE", "DOCS_ONLY_FIRMWARE_BINARY", "DOCS_ONLY_RELEASE_PACKAGE"):
                self.assertIn(code, result.stdout)


class WrapperCommandTests(unittest.TestCase):
    def test_exact_wrapper_invocation_and_docs_only_output(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            config = paired_config(root)
            write_files(root, {"README.md": "[中文](README_CN.md)\n# Guide\n", "README_CN.md": "[English](README.md)\n# 指南\n"})
            changed = changed_file(root, ["M\tREADME.md", "M\tREADME_CN.md"])
            output = root / "github-output.txt"
            env = os.environ.copy()
            env["GITHUB_OUTPUT"] = str(output)
            result = run([sys.executable, str(WRAPPER), "--repo", str(root), "--changed-files-from", str(changed), "--config", str(config), "--format", "json"], env=env)
            self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
            self.assertIn("docs_only=true", output.read_text(encoding="utf-8"))

    def test_wrapper_strict_propagates_warnings(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            config = paired_config(root)
            write_files(
                root,
                {
                    "README.md": "[中文](README_CN.md)\n# English\n",
                    "README_CN.md": "[English](README.md)\n# 中文\n",
                    "NOTES.md": "[简体中文](NOTES_CN.md)\n# Notes\n",
                    "NOTES_CN.md": "[English](NOTES.md)\n# 说明\n",
                },
            )
            changed = changed_file(root, ["M\tNOTES.md", "M\tNOTES_CN.md"])
            result = run(
                [sys.executable, str(WRAPPER), "--repo", str(root), "--changed-files-from", str(changed), "--config", str(config), "--strict", "--format", "json"]
            )
            self.assertEqual(result.returncode, 1)
            self.assertIn("NONSTANDARD_FIRST_PARTY_LANGUAGE_SUFFIX", result.stdout)

    def test_empty_and_malformed_changed_lists_return_operational_error(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            config = paired_config(root)
            for content in ("", "not-a-name-status-line\n"):
                changed = root / "changed.txt"
                changed.write_text(content, encoding="utf-8")
                result = run([sys.executable, str(WRAPPER), "--repo", str(root), "--changed-files-from", str(changed), "--config", str(config), "--format", "json"])
                self.assertEqual(result.returncode, 2)


if __name__ == "__main__":
    unittest.main()
