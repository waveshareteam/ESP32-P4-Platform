#!/usr/bin/env python3
"""Focused offline contracts for the product firmware packager."""

from __future__ import annotations

import hashlib
import json
import shlex
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[3]
PACKAGER = REPO_ROOT / ".github" / "scripts" / "package_product_firmware.py"
SHA = "a" * 40


class ProductFirmwarePackagerTests(unittest.TestCase):
    def fixture(self, profile: str = "rev1_3") -> tuple[tempfile.TemporaryDirectory[str], Path, Path, Path, Path]:
        temporary = tempfile.TemporaryDirectory()
        root = Path(temporary.name)
        project = root / "project"
        build = root / "build"
        project.mkdir()
        (build / "bootloader").mkdir(parents=True)
        (build / "partition_table").mkdir()
        (build / "app.bin").write_bytes(b"APP")
        (build / "bootloader" / "bootloader.bin").write_bytes(b"BOOT")
        (build / "partition_table" / "partition-table.bin").write_bytes(b"PART")
        (build / "project_description.json").write_text(json.dumps({"target": "esp32p4"}), encoding="utf-8")
        (build / "flasher_args.json").write_text(
            json.dumps(
                {
                    "flash_files": {"0x1000": "bootloader/bootloader.bin", "0x8000": "partition_table/partition-table.bin", "0x10000": "app.bin"},
                    "flash_settings": {"flash_mode": "dio", "flash_freq": "80m", "flash_size": "32MB"},
                }
            ),
            encoding="utf-8",
        )
        config_lines = ["CONFIG_ESPTOOLPY_FLASHSIZE_32MB=y"]
        if profile == "rev1_3":
            config_lines.extend(("CONFIG_ESP32P4_SELECTS_REV_LESS_V3=y", "CONFIG_ESP32P4_REV_MIN_100=y"))
        else:
            config_lines.extend(("CONFIG_ESP32P4_SELECTS_REV_LESS_V3=n", "CONFIG_ESP32P4_REV_MIN_300=y"))
        sdkconfig = root / "sdkconfig"
        sdkconfig.write_text("\n".join(config_lines) + "\n", encoding="utf-8")
        return temporary, project, build, sdkconfig, root / "output"

    def run_packager(self, project: Path, build: Path, sdkconfig: Path, output: Path, profile: str = "rev1_3", source_sha: str = SHA, pr_head_sha: str = SHA, event: str = "pull_request", run_id: str = "123", run_attempt: str = "1") -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [
                sys.executable, str(PACKAGER),
                "--project", str(project), "--build-dir", str(build), "--sdkconfig", str(sdkconfig),
                "--profile", profile, "--output-dir", str(output), "--source-sha", source_sha,
                "--pr-head-sha", pr_head_sha, "--event", event, "--run-id", run_id,
                "--run-attempt", run_attempt, "--idf-version", "v5.5.5",
            ],
            cwd=REPO_ROOT,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

    def assert_failure(self, mutate, expected: str, profile: str = "rev1_3") -> None:
        temporary, project, build, sdkconfig, output = self.fixture(profile)
        with temporary:
            mutate(build, sdkconfig)
            result = self.run_packager(project, build, sdkconfig, output, profile)
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn(expected, result.stderr)

    def test_rev1_3_success_has_exact_merged_image_and_safe_helpers(self) -> None:
        temporary, project, build, sdkconfig, output = self.fixture("rev1_3")
        with temporary:
            result = self.run_packager(project, build, sdkconfig, output)
            self.assertEqual(result.returncode, 0, result.stderr)
            manifest = json.loads((output / "manifest.json").read_text(encoding="utf-8"))
            merged = output / "merged.bin"
            self.assertEqual(merged.stat().st_size, 32 * 1024 * 1024)
            self.assertEqual(manifest["schema_version"], 2)
            self.assertEqual(manifest["profile"], "rev1_3")
            self.assertEqual(manifest["capacity"], "32MiB")
            self.assertFalse(manifest["c6_binary_included"])
            self.assertFalse(manifest["erase_required"])
            self.assertEqual(manifest["flash_by_default"], "split_files")
            self.assertFalse(manifest["merged_image"]["flash_by_default"])
            self.assertEqual(manifest["merged_image"]["sha256"], hashlib.sha256(merged.read_bytes()).hexdigest())
            self.assertEqual(merged.read_bytes()[0x10000:0x10003], b"APP")
            self.assertEqual(merged.read_bytes()[0x10003], 0xFF)
            for helper in ("flash_args", "flash.sh", "flash.bat", "README.md"):
                helper_text = (output / helper).read_text(encoding="utf-8")
                self.assertIn("write_flash", helper_text)
                self.assertNotIn("erase_flash", helper_text)
            expected_prefix = ["--chip", "esp32p4", "write_flash", "--flash_mode", "dio", "--flash_freq", "80m", "--flash_size", "32MB"]
            self.assertEqual((output / "flash_args").read_text(encoding="utf-8").split()[:9], expected_prefix)
            self.assertEqual(shlex.split((output / "flash.sh").read_text(encoding="utf-8").splitlines()[-1])[1:10], expected_prefix)
            self.assertEqual((output / "flash.bat").read_text(encoding="utf-8").splitlines()[-1].split()[1:10], expected_prefix)
            self.assertFalse(any(line.startswith("+") for line in (output / "README.md").read_text(encoding="utf-8").splitlines()))
            self.assertIn("bin/00010000_app.bin", (output / "SHA256SUMS").read_text(encoding="utf-8"))

    def test_rev3_x_success_accepts_unset_revision_selector(self) -> None:
        temporary, project, build, sdkconfig, output = self.fixture("rev3_x")
        with temporary:
            sdkconfig.write_text("CONFIG_ESPTOOLPY_FLASHSIZE_32MB=y\nCONFIG_ESP32P4_REV_MIN_300=y\n", encoding="utf-8")
            result = self.run_packager(project, build, sdkconfig, output, "rev3_x")
            self.assertEqual(result.returncode, 0, result.stderr)
            manifest = json.loads((output / "manifest.json").read_text(encoding="utf-8"))
            self.assertEqual(manifest["revision_contract"]["revision_minimum"], "300")

    def test_wrong_target_fails_closed(self) -> None:
        self.assert_failure(lambda build, _: (build / "project_description.json").write_text('{"target":"esp32c6"}', encoding="utf-8"), "target must be esp32p4")

    def test_profile_config_mismatch_fails_closed(self) -> None:
        self.assert_failure(lambda _, config: config.write_text("CONFIG_ESPTOOLPY_FLASHSIZE_32MB=y\nCONFIG_ESP32P4_REV_MIN_300=y\n", encoding="utf-8"), "rev1_3 revision contract")

    def test_c6_named_image_fails_closed(self) -> None:
        def mutate(build: Path, _: Path) -> None:
            (build / "esp32c6.bin").write_bytes(b"bad")
            data = json.loads((build / "flasher_args.json").read_text(encoding="utf-8"))
            data["flash_files"]["0x20000"] = "esp32c6.bin"
            (build / "flasher_args.json").write_text(json.dumps(data), encoding="utf-8")
        self.assert_failure(mutate, "C6-named")

        def c6_target(build: Path, _: Path) -> None:
            data = json.loads((build / "flasher_args.json").read_text(encoding="utf-8"))
            data["target"] = "esp32c6"
            (build / "flasher_args.json").write_text(json.dumps(data), encoding="utf-8")
        self.assert_failure(c6_target, "target must be esp32p4")

    def test_path_traversal_and_outside_paths_fail_closed(self) -> None:
        def traversal(build: Path, _: Path) -> None:
            data = json.loads((build / "flasher_args.json").read_text(encoding="utf-8"))
            data["flash_files"] = {"0x1000": "../outside.bin"}
            (build / "flasher_args.json").write_text(json.dumps(data), encoding="utf-8")
        self.assert_failure(traversal, "unsafe flash file path")

        def outside(build: Path, _: Path) -> None:
            outside_file = build.parent / "outside.bin"
            outside_file.write_bytes(b"outside")
            data = json.loads((build / "flasher_args.json").read_text(encoding="utf-8"))
            data["flash_files"] = {"0x1000": "linked.bin"}
            (build / "linked.bin").symlink_to(outside_file)
            (build / "flasher_args.json").write_text(json.dumps(data), encoding="utf-8")
        self.assert_failure(outside, "escapes build directory")

    def test_overlap_out_of_bounds_duplicate_missing_and_empty_fail_closed(self) -> None:
        def overlap(build: Path, _: Path) -> None:
            data = json.loads((build / "flasher_args.json").read_text(encoding="utf-8"))
            data["flash_files"] = {"0x1000": "bootloader/bootloader.bin", "0x1002": "app.bin"}
            (build / "flasher_args.json").write_text(json.dumps(data), encoding="utf-8")
        self.assert_failure(overlap, "overlaps")

        def bounds(build: Path, _: Path) -> None:
            data = json.loads((build / "flasher_args.json").read_text(encoding="utf-8"))
            data["flash_files"] = {"0x1ffffff": "app.bin"}
            (build / "flasher_args.json").write_text(json.dumps(data), encoding="utf-8")
        self.assert_failure(bounds, "exceeds 32 MiB")

        def duplicate(build: Path, _: Path) -> None:
            data = json.loads((build / "flasher_args.json").read_text(encoding="utf-8"))
            data["flash_files"] = {"0x1000": "app.bin", "4096": "bootloader/bootloader.bin"}
            (build / "flasher_args.json").write_text(json.dumps(data), encoding="utf-8")
        self.assert_failure(duplicate, "duplicate flash offset")

        def missing(build: Path, _: Path) -> None:
            data = json.loads((build / "flasher_args.json").read_text(encoding="utf-8"))
            data["flash_files"] = {"0x1000": "missing.bin"}
            (build / "flasher_args.json").write_text(json.dumps(data), encoding="utf-8")
        self.assert_failure(missing, "missing flash file")

        def empty(build: Path, _: Path) -> None:
            (build / "empty.bin").write_bytes(b"")
            data = json.loads((build / "flasher_args.json").read_text(encoding="utf-8"))
            data["flash_files"] = {"0x1000": "empty.bin"}
            (build / "flasher_args.json").write_text(json.dumps(data), encoding="utf-8")
        self.assert_failure(empty, "non-empty regular file")

    def test_invalid_or_mismatched_identity_fails_closed(self) -> None:
        temporary, project, build, sdkconfig, output = self.fixture()
        with temporary:
            result = self.run_packager(project, build, sdkconfig, output, source_sha="short")
            self.assertEqual(result.returncode, 2)
            self.assertIn("40-character", result.stderr)
            result = self.run_packager(project, build, sdkconfig, output, source_sha="a" * 64)
            self.assertEqual(result.returncode, 2)
            self.assertIn("40-character", result.stderr)
            result = self.run_packager(project, build, sdkconfig, output, pr_head_sha="b" * 40)
            self.assertEqual(result.returncode, 2)
            self.assertIn("same revision", result.stderr)
            result = self.run_packager(project, build, sdkconfig, output, event="schedule")
            self.assertEqual(result.returncode, 2)
            self.assertIn("event must be one of", result.stderr)
            result = self.run_packager(project, build, sdkconfig, output, run_id="0")
            self.assertEqual(result.returncode, 2)
            self.assertIn("run-id must be a positive decimal integer", result.stderr)
            result = self.run_packager(project, build, sdkconfig, output, run_attempt="01")
            self.assertEqual(result.returncode, 2)
            self.assertIn("run-attempt must be a positive decimal integer", result.stderr)


if __name__ == "__main__":
    unittest.main()
