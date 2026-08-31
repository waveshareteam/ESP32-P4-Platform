#!/usr/bin/env python3
"""Offline contracts for product firmware profile discovery and workflow wiring."""

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
SELECTOR = SCRIPTS_ROOT / "discover_firmware_profiles.py"
WORKFLOW = REPO_ROOT / ".github" / "workflows" / "product-firmware.yml"


def run_selector(*args: str, changed: str | None = None) -> tuple[subprocess.CompletedProcess[str], str]:
    with tempfile.TemporaryDirectory() as temp_dir:
        temp = Path(temp_dir)
        output = temp / "github-output.txt"
        command = [sys.executable, str(SELECTOR), *args]
        if changed is not None:
            changed_file = temp / "changed-files.txt"
            changed_file.write_text(changed, encoding="utf-8")
            command.extend(("--changed-files-from", str(changed_file)))
        environment = os.environ.copy()
        environment["GITHUB_OUTPUT"] = str(output)
        result = subprocess.run(
            command,
            cwd=REPO_ROOT,
            env=environment,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        return result, output.read_text(encoding="utf-8") if output.exists() else ""


class FirmwareSelectorTests(unittest.TestCase):
    def assert_profiles(self, changed: str, expected: list[str]) -> None:
        result, output = run_selector(changed=changed)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout), {"profile": expected})
        self.assertIn(f"has_profiles={str(bool(expected)).lower()}", output)

    def test_manual_profiles(self) -> None:
        result, _ = run_selector("--profile", "rev1_3")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout), {"profile": ["rev1_3"]})

    def test_source_documentation_and_factory_routing(self) -> None:
        self.assert_profiles("M\tfirmware/brookesia/main/main.cpp\n", ["rev1_3", "rev3_x"])
        self.assert_profiles("M\tdocs/firmware.md\n", [])
        factory = "M\tfirmware/factory_firmware/factory.bin\n"
        result, output = run_selector(changed=factory)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout), {"profile": []})
        self.assertIn('firmware_paths=["firmware/factory_firmware/factory.bin"]', output)
        self.assertIn('release_paths=["firmware/factory_firmware/factory.bin"]', output)

    def test_known_non_product_paths_do_not_build_firmware(self) -> None:
        known_non_product = (
            "M\texamples/esp-idf/02_HelloWorld/main/hello_world_main.c\n",
            "M\texamples/arduino/examples/HelloWorld/HelloWorld.ino\n",
            "M\t.github/workflows/esp-idf-examples.yml\n",
            "M\tconfig/esp32p4_local_defaults.cmake\n",
            "M\tconfig/esp32p4_rev1_3.defaults\n",
            "M\t.gitignore\n",
            "M\t.github/scripts/discover_firmware_profiles.py\n",
            "M\t.github/scripts/tests/test_firmware_routing.py\n",
            "M\tscripts/Flash-CI-Firmware.ps1\n",
            "M\tFlash-CI-Firmware.cmd\n",
        )
        for changed in known_non_product:
            with self.subTest(changed=changed):
                self.assert_profiles(changed, [])

    def test_arduino_only_ci_inputs_are_explicitly_non_product(self) -> None:
        paths = (
            ".github/workflows/arduino-examples.yml",
            ".github/scripts/discover_arduino_examples.py",
            ".github/scripts/package_arduino_firmware.py",
            ".github/scripts/tests/test_package_arduino_firmware.py",
            ".github/scripts/tests/test_arduino_serial_readiness.py",
            "examples/arduino/common/serial_log.h",
        )
        for path in paths:
            with self.subTest(path=path):
                result, output = run_selector(changed=f"M\t{path}\n")
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(json.loads(result.stdout), {"profile": []})
                self.assertIn("has_profiles=false", output)
                self.assertIn("unknown_paths=[]", output)

    def test_evidenced_shared_inputs_build_both_profiles(self) -> None:
        product_inputs = (
            "M\t.github/workflows/product-firmware.yml\n",
            "M\t.github/scripts/package_product_firmware.py\n",
        )
        for changed in product_inputs:
            with self.subTest(changed=changed):
                self.assert_profiles(changed, ["rev1_3", "rev3_x"])

    def test_unknown_non_document_path_fails_closed_without_profiles(self) -> None:
        result, output = run_selector(changed="M\tnew-build-system/tool.py\n")
        self.assertEqual(result.returncode, 2)
        self.assertEqual(json.loads(result.stdout), {"profile": []})
        self.assertIn("has_profiles=false", output)
        self.assertIn('unknown_paths=["new-build-system/tool.py"]', output)
        self.assertIn("unclassified non-document paths", result.stderr)

    def test_unmanaged_firmware_project_fails_closed(self) -> None:
        result, output = run_selector(changed="M\tfirmware/other/main/main.c\n")
        self.assertEqual(result.returncode, 2)
        self.assertEqual(json.loads(result.stdout), {"profile": []})
        self.assertIn('unknown_paths=["firmware/other/main/main.c"]', output)

    def test_mixed_product_and_unknown_scope_fails_without_building(self) -> None:
        result, output = run_selector(
            changed=(
                "M\tfirmware/brookesia/main/main.cpp\n"
                "M\tnew-build-system/tool.py\n"
            )
        )
        self.assertEqual(result.returncode, 2)
        self.assertEqual(json.loads(result.stdout), {"profile": []})
        self.assertIn("has_profiles=false", output)

    def test_rename_keeps_old_and_new_paths(self) -> None:
        result, output = run_selector(
            changed="R100\tfirmware/brookesia/main/old.cpp\tdocs/new.md\n"
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout), {"profile": ["rev1_3", "rev3_x"]})
        self.assertIn("changed_path_count=2", output)

    def test_empty_diff_fails_closed(self) -> None:
        result, _ = run_selector(changed="")
        self.assertEqual(result.returncode, 2)
        self.assertIn("empty", result.stderr)

    def test_base_ref_uses_a_diverged_dag_and_missing_base_fails(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)

            def git(*args: str) -> str:
                result = subprocess.run(
                    ["git", *args],
                    cwd=root,
                    check=True,
                    text=True,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                )
                return result.stdout.strip()

            git("init", "-b", "main")
            git("config", "user.name", "CI Test")
            git("config", "user.email", "ci-test@example.invalid")
            source = root / "firmware" / "brookesia" / "main" / "main.cpp"
            source.parent.mkdir(parents=True)
            source.write_text("int main() { return 0; }\n", encoding="utf-8")
            (root / "README.md").write_text("# Initial\n", encoding="utf-8")
            git("add", ".")
            git("commit", "-m", "initial")

            git("switch", "-c", "feature")
            source.write_text("int main() { return 1; }\n", encoding="utf-8")
            git("add", ".")
            git("commit", "-m", "feature")
            head_sha = git("rev-parse", "HEAD")

            git("switch", "main")
            (root / "README.md").write_text("# Base advanced\n", encoding="utf-8")
            git("add", "README.md")
            git("commit", "-m", "advance base")
            base_sha = git("rev-parse", "HEAD")

            result = subprocess.run(
                [
                    sys.executable,
                    str(SELECTOR),
                    "--base-ref",
                    base_sha,
                    "--head-ref",
                    head_sha,
                ],
                cwd=root,
                check=False,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(
                json.loads(result.stdout),
                {"profile": ["rev1_3", "rev3_x"]},
            )

            missing = subprocess.run(
                [
                    sys.executable,
                    str(SELECTOR),
                    "--base-ref",
                    "0" * 40,
                    "--head-ref",
                    head_sha,
                ],
                cwd=root,
                check=False,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            self.assertEqual(missing.returncode, 2)
            self.assertIn("invalid symmetric difference", missing.stderr.lower())


class ProductFirmwareWorkflowTests(unittest.TestCase):
    def test_workflow_contract(self) -> None:
        workflow = WORKFLOW.read_text(encoding="utf-8")
        self.assertIn("name: Product firmware", workflow)
        self.assertIn("- rev1_3", workflow)
        self.assertIn("- rev3_x", workflow)
        self.assertIn("fetch-depth: 0", workflow)
        self.assertIn("esp_idf_version: v5.5.5", workflow)
        self.assertIn("path: firmware/brookesia", workflow)
        self.assertIn("espressif/esp-idf-ci-action@v1", workflow)
        self.assertIn("../../.ci-build/firmware/${{ matrix.profile }}/build", workflow)
        self.assertIn("../../.ci-build/firmware/${{ matrix.profile }}/sdkconfig", workflow)
        self.assertIn('-D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.${{ matrix.profile }}.defaults"', workflow)
        self.assertIn("package_product_firmware.py", workflow)
        self.assertIn('PR_HEAD_SHA: ${{ github.event.pull_request.head.sha || github.sha }}', workflow)
        self.assertNotIn("SOURCE_SHA: ${{", workflow)
        self.assertIn('SOURCE_SHA="$(git rev-parse HEAD)"\n          python .github/scripts/package_product_firmware.py', workflow)
        self.assertIn(".ci-artifacts/firmware/${{ matrix.profile }}", workflow)
        self.assertIn("esp32-p4-platform-firmware-${{ matrix.profile }}-${{ github.event.pull_request.head.sha || github.sha }}", workflow)
        self.assertIn("retention-days: 14", workflow)
        self.assertIn("compression-level: 0", workflow)
        self.assertIn("if: always()", workflow)
        self.assertIn("fetch-depth: 0", workflow)
        self.assertIn("PR_BASE_REF: ${{ github.event.pull_request.base.ref }}", workflow)
        self.assertIn('canonical_base="refs/remotes/canonical/${PR_BASE_REF}"', workflow)
        self.assertIn('git merge-base "$PR_BASE_SHA" "$PR_HEAD_SHA" >/dev/null', workflow)
        self.assertNotIn("--depth=1", workflow)


if __name__ == "__main__":
    unittest.main()
