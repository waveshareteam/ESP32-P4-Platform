#!/usr/bin/env python3
"""Synthetic acceptance tests for repository example CI routing."""

from __future__ import annotations

import json
import os
import re
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path, PurePosixPath


REPO_ROOT = Path(__file__).resolve().parents[3]
SCRIPTS_ROOT = REPO_ROOT / ".github" / "scripts"
sys.path.insert(0, str(SCRIPTS_ROOT))

from ci_change_routing import (  # noqa: E402
    Change,
    ChangeListError,
    parse_name_status,
    route_changes,
)
import discover_arduino_examples as arduino  # noqa: E402
import discover_esp_idf_examples as esp_idf  # noqa: E402


IDF_EXAMPLES = {
    f"examples/esp-idf/{index:02d}_{name}"
    for index, name in enumerate(
        [
            "board_check",
            "HowToCreateProject",
            "HelloWorld",
            "nvs_counter",
            "freertos_tasks",
            "gpio_io",
            "gpio_interrupt",
            "uart_loopback",
            "i2c_tools",
            "sdmmc",
            "wifistation",
            "ethernetbasic",
            "I2SCodec",
            "Displaycolorbar",
            "lvgl_demo_v9",
            "eth2ap",
            "video_lcd_display",
            "simple_video_server",
            "esp_brookesia_phone",
            "system_monitor",
        ]
    )
}
ARDUINO_SKETCHES = {
    f"examples/arduino/examples/{name}"
    for name in (
        "AsciiTable",
        "Audio_Playback",
        "Camera_ISP_Tuning",
        "Camera_Preview",
        "Drawing_board",
        "GFX_ESPWiFiAnalyzer",
        "HelloWorld",
        "LVGLV9_Arduino",
        "SD_Card",
    )
}


def change(path: str, status: str = "M", *, deleted: bool = False) -> Change:
    return Change(status=status, path=path, deleted=deleted)


class NameStatusTests(unittest.TestCase):
    def test_rename_keeps_both_paths(self) -> None:
        parsed = parse_name_status(["R100\told/path.c\tnew/path.c"])
        self.assertEqual([item.path for item in parsed], ["old/path.c", "new/path.c"])
        self.assertTrue(parsed[0].deleted)
        self.assertFalse(parsed[1].deleted)

    def test_empty_input_fails_closed(self) -> None:
        with self.assertRaises(ChangeListError):
            parse_name_status([])


class InventoryTests(unittest.TestCase):
    def test_expected_repository_counts(self) -> None:
        self.assertEqual(set(esp_idf.list_examples()), IDF_EXAMPLES)
        self.assertEqual(set(arduino.list_sketches()), ARDUINO_SKETCHES)

    def test_arduino_discovery_excludes_bundled_library_examples(self) -> None:
        bundled_examples = list(
            (REPO_ROOT / "examples" / "arduino" / "libraries").rglob("*.ino")
        )
        discovered = set(arduino.list_sketches())

        self.assertTrue(bundled_examples, "expected bundled Arduino library examples")
        self.assertTrue(
            all(
                not entry.startswith("examples/arduino/libraries/")
                for entry in discovered
            ),
            "bundled library examples must not become first-party CI sketches",
        )

    def test_lvgl_demo_v9_direct_component_dependencies(self) -> None:
        project = REPO_ROOT / "examples" / "esp-idf" / "14_lvgl_demo_v9" / "main"
        main_text = (project / "main.c").read_text(encoding="utf-8")
        cmake_text = (project / "CMakeLists.txt").read_text(encoding="utf-8")
        direct_component_headers = {
            "nvs_flash.h": "nvs_flash",
            "nvs.h": "nvs_flash",
            "esp_memory_utils.h": "esp_mm",
        }
        requires_match = re.search(
            r"(?ms)^\s*REQUIRES\s+(.+?)\s*\)", cmake_text
        )

        self.assertIsNotNone(requires_match, "missing REQUIRES")
        required_components = set(requires_match.group(1).split())
        for header, component in direct_component_headers.items():
            with self.subTest(header=header, component=component):
                self.assertIn(f'#include "{header}"', main_text)
                self.assertIn(component, required_components)

    def test_idf_projects_default_to_rev3_x_and_allow_explicit_rev1_3(self) -> None:
        helper = REPO_ROOT / "config" / "esp32p4_local_defaults.cmake"
        helper_text = helper.read_text(encoding="utf-8")
        projects = sorted(esp_idf.list_examples())
        helper_include = (
            "include(${CMAKE_CURRENT_LIST_DIR}/../../../config/"
            "esp32p4_local_defaults.cmake)"
        )
        idf_project_include = "include($ENV{IDF_PATH}/tools/cmake/project.cmake)"

        self.assertEqual(len(projects), 20)
        self.assertEqual(set(projects), IDF_EXAMPLES)
        rev1_3_overlay = helper.parent / "esp32p4_rev1_3.defaults"
        rev3_x_overlay = helper.parent / "esp32p4_rev3_x.defaults"
        self.assertTrue(rev1_3_overlay.is_file())
        self.assertTrue(rev3_x_overlay.is_file())
        self.assertIn("NOT DEFINED SDKCONFIG_DEFAULTS", helper_text)
        self.assertIn("NOT DEFINED ENV{SDKCONFIG_DEFAULTS}", helper_text)
        self.assertIn("sdkconfig.defaults;${CMAKE_CURRENT_LIST_DIR}", helper_text)
        self.assertIn("/esp32p4_rev3_x.defaults", helper_text)
        self.assertNotIn("/esp32p4_rev1_3.defaults", helper_text)
        self.assertNotIn("sdkconfig.defaults.esp32p4", helper_text)
        # The default helper is intentionally conditional: callers that need
        # the earlier silicon profile can explicitly provide the rev1.3
        # overlay without the helper replacing their selection.
        self.assertRegex(
            helper_text,
            r"\(NOT DEFINED SDKCONFIG_DEFAULTS OR SDKCONFIG_DEFAULTS STREQUAL \"\"\)"
            r"\s+AND\s+"
            r"\(NOT DEFINED ENV\{SDKCONFIG_DEFAULTS\} OR \"\$ENV\{SDKCONFIG_DEFAULTS\}\" STREQUAL \"\"\)",
        )

        for project in projects:
            with self.subTest(project=project):
                self.assertTrue((REPO_ROOT / project / "sdkconfig.defaults").is_file())
                cmake_text = (REPO_ROOT / project / "CMakeLists.txt").read_text(
                    encoding="utf-8"
                )
                self.assertIn(helper_include, cmake_text)
                self.assertIn(idf_project_include, cmake_text)
                self.assertLess(
                    cmake_text.index(helper_include),
                    cmake_text.index(idf_project_include),
                )

    def test_brookesia_product_profile_contracts(self) -> None:
        brookesia_root = REPO_ROOT / "firmware" / "brookesia"
        main_text = (brookesia_root / "main" / "main.cpp").read_text(encoding="utf-8")
        cmake_text = (brookesia_root / "main" / "CMakeLists.txt").read_text(
            encoding="utf-8"
        )
        rev1_3_text = (brookesia_root / "sdkconfig.rev1_3.defaults").read_text(
            encoding="utf-8"
        )
        rev3_x_text = (brookesia_root / "sdkconfig.rev3_x.defaults").read_text(
            encoding="utf-8"
        )

        component_register = cmake_text.partition("idf_component_register(")[2].partition(
            ")"
        )[0]

        def dependencies(scope: str) -> set[str]:
            match = re.search(
                rf"(?ms)^\s*{scope}\s+(.+?)(?=^\s*(?:REQUIRES|PRIV_REQUIRES)\b|\Z)",
                component_register,
            )
            self.assertIsNotNone(match, f"missing {scope}")
            return set(match.group(1).split())

        public_dependencies = dependencies("REQUIRES")
        private_dependencies = dependencies("PRIV_REQUIRES")
        direct_component_headers = {
            "Drawpanel.hpp": "draw",
            "SpecAnalyzer.hpp": "SpecAnalyzer",
            "MusicPlayer.hpp": "MusicPlayer",
            "Settings.hpp": "Settings",
            "Camera.hpp": "Camera",
            "VideoPlayer.hpp": "VideoPlayer",
            "esp_brookesia_app_calculator.hpp": "brookesia_app_calculator",
        }
        system_headers = {
            "esp_ota_ops.h": "app_update",
            "nvs_flash.h": "nvs_flash",
            "esp_wifi.h": "esp_wifi",
        }

        self.assertIn('#include "esp_brookesia.hpp"', main_text)
        self.assertEqual(public_dependencies, {"product_support", "brookesia_core"})
        self.assertEqual(
            private_dependencies,
            set(system_headers.values()) | set(direct_component_headers.values()),
        )
        for header, dependency in {**system_headers, **direct_component_headers}.items():
            with self.subTest(header=header, dependency=dependency):
                self.assertIn(f'#include "{header}"', main_text)
                self.assertIn(dependency, private_dependencies)
        self.assertIn("CONFIG_BOOTLOADER_LOG_LEVEL_ERROR=y", rev3_x_text)
        self.assertIn("CONFIG_BOOTLOADER_LOG_LEVEL=1", rev3_x_text)
        self.assertNotIn("CONFIG_BOOTLOADER_LOG_LEVEL", rev1_3_text)
        self.assertNotIn("CONFIG_PARTITION_TABLE_OFFSET", rev3_x_text)

    def test_esp_idf_workflow_uses_container_safe_local_defaults(self) -> None:
        workflow = (
            REPO_ROOT / ".github" / "workflows" / "esp-idf-examples.yml"
        ).read_text(encoding="utf-8")

        self.assertIn(
            'SDKCONFIG_DEFAULTS="sdkconfig.defaults;../../../config/esp32p4_rev3_x.defaults"',
            workflow,
        )
        self.assertNotIn("github.workspace", workflow)


class RoutingTests(unittest.TestCase):
    def route_idf(self, paths: list[Change]):
        return route_changes(
            paths,
            known_entries=IDF_EXAMPLES,
            root=PurePosixPath("examples/esp-idf"),
            other_framework_root=PurePosixPath("examples/arduino"),
            global_patterns=esp_idf.GLOBAL_EXAMPLE_PATTERNS,
            other_framework_patterns=esp_idf.ARDUINO_ONLY_PATTERNS,
        )

    def route_arduino(self, paths: list[Change]):
        return route_changes(
            paths,
            known_entries=ARDUINO_SKETCHES,
            root=PurePosixPath("examples/arduino/examples"),
            other_framework_root=PurePosixPath("examples/esp-idf"),
            global_patterns=arduino.GLOBAL_SKETCH_PATTERNS,
            other_framework_patterns=arduino.ESP_IDF_ONLY_PATTERNS,
        )

    def test_documentation_selects_no_examples(self) -> None:
        documentation = [
            "README.md",
            "SECURITY.md",
            "examples/esp-idf/00_board_check/README.md",
            "examples/arduino/examples/HelloWorld/README.md",
            "examples/arduino/libraries/GFX_Library_for_Arduino/README.md",
        ]
        for path in documentation:
            with self.subTest(path=path):
                self.assertEqual(self.route_idf([change(path)]).selected, ())
                self.assertEqual(self.route_arduino([change(path)]).selected, ())

    def test_policy_paths_skip_example_builds_but_are_not_docs_only(self) -> None:
        policy_paths = [
            ".github/workflows/repository-policy.yml",
            ".github/scripts/audit_markdown.py",
            ".github/scripts/run_markdown_audit.py",
            ".github/scripts/tests/test_markdown_audit.py",
            "config/markdown-audit.json",
        ]
        for router in (self.route_idf, self.route_arduino):
            result = router([change(path) for path in policy_paths])
            self.assertEqual(result.selected, ())
            self.assertFalse(result.docs_only)

    def test_discovery_routing_test_still_selects_full_matrices(self) -> None:
        path = ".github/scripts/tests/test_ci_discovery.py"
        idf_result = self.route_idf([change(path)])
        arduino_result = self.route_arduino([change(path)])
        self.assertEqual(set(idf_result.selected), IDF_EXAMPLES)
        self.assertEqual(set(arduino_result.selected), ARDUINO_SKETCHES)
        self.assertEqual(idf_result.unknown_paths, ())
        self.assertEqual(arduino_result.unknown_paths, ())

    def test_arduino_only_inputs_select_all_nine_sketches_and_skip_idf(self) -> None:
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
                arduino_result = self.route_arduino([change(path)])
                idf_result = self.route_idf([change(path)])
                self.assertEqual(set(arduino_result.selected), ARDUINO_SKETCHES)
                self.assertEqual(arduino_result.unknown_paths, ())
                self.assertEqual(idf_result.selected, ())
                self.assertEqual(idf_result.unknown_paths, ())

    def test_idf_only_inputs_select_idf_and_skip_arduino(self) -> None:
        paths = (
            ".github/workflows/esp-idf-examples.yml",
            ".github/scripts/discover_esp_idf_examples.py",
            "config/esp32p4_local_defaults.cmake",
            "config/esp32p4_rev1_3.defaults",
            "config/esp32p4_rev3_x.defaults",
        )
        for path in paths:
            with self.subTest(path=path):
                idf_result = self.route_idf([change(path)])
                arduino_result = self.route_arduino([change(path)])
                self.assertEqual(set(idf_result.selected), IDF_EXAMPLES)
                self.assertEqual(idf_result.unknown_paths, ())
                self.assertEqual(arduino_result.selected, ())
                self.assertEqual(arduino_result.unknown_paths, ())

    def test_direct_source_selects_only_affected_entry(self) -> None:
        idf_result = self.route_idf(
            [change("examples/esp-idf/02_HelloWorld/main/hello_world_main.c")]
        )
        self.assertEqual(idf_result.selected, ("examples/esp-idf/02_HelloWorld",))
        arduino_result = self.route_arduino(
            [change("examples/arduino/examples/HelloWorld/HelloWorld.ino")]
        )
        self.assertEqual(
            arduino_result.selected,
            ("examples/arduino/examples/HelloWorld",),
        )

    def test_shared_inputs_select_the_framework_matrix(self) -> None:
        for path in (
            "config/esp32p4_rev1_3.defaults",
            "config/esp32p4_local_defaults.cmake",
        ):
            with self.subTest(path=path):
                idf_result = self.route_idf([change(path)])
                self.assertEqual(set(idf_result.selected), IDF_EXAMPLES)
        arduino_result = self.route_arduino(
            [
                change(
                    "examples/arduino/libraries/GFX_Library_for_Arduino/src/Arduino_GFX.cpp"
                )
            ]
        )
        self.assertEqual(set(arduino_result.selected), ARDUINO_SKETCHES)

    def test_workflow_input_selects_all_for_that_framework(self) -> None:
        result = self.route_idf([change(".github/workflows/esp-idf-examples.yml")])
        self.assertEqual(set(result.selected), IDF_EXAMPLES)

    def test_firmware_kinds_are_reported_but_not_built(self) -> None:
        firmware_paths = [
            "firmware/README.md",
            "firmware/brookesia/main/main.cpp",
            "firmware/brookesia/main/idf_component.yml",
            "firmware/factory_firmware/factory.bin",
            "firmware/factory_firmware/delivery.zip",
        ]
        for router in (self.route_idf, self.route_arduino):
            result = router([change(path) for path in firmware_paths])
            self.assertEqual(result.selected, ())
            self.assertEqual(set(result.firmware_paths), set(firmware_paths))

    def test_rename_and_deletion_keep_old_path_impact(self) -> None:
        renamed = parse_name_status(
            [
                "R100\texamples/arduino/examples/HelloWorld/old.cpp\t"
                "examples/arduino/examples/HelloWorld/new.cpp"
            ]
        )
        result = self.route_arduino(renamed)
        self.assertEqual(result.selected, ("examples/arduino/examples/HelloWorld",))

        removed = self.route_idf(
            [
                change(
                    "examples/esp-idf/20_removed/CMakeLists.txt",
                    status="D",
                    deleted=True,
                )
            ]
        )
        self.assertEqual(removed.removed_entries, ("examples/esp-idf/20_removed",))
        self.assertEqual(set(removed.selected), IDF_EXAMPLES)

    def test_unknown_non_document_path_is_conservative(self) -> None:
        result = self.route_idf([change("new-build-system/tool.py")])
        self.assertEqual(set(result.selected), IDF_EXAMPLES)
        self.assertEqual(result.unknown_paths, ("new-build-system/tool.py",))


class CommandLineTests(unittest.TestCase):
    def assert_selector_paths(
        self,
        *,
        script: str,
        matrix_key: str,
        paths: tuple[str, ...],
        expected: set[str],
    ) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            temp = Path(temp_dir)
            changed = temp / "changed-files.txt"
            output = temp / "github-output.txt"
            changed.write_text(
                "".join(f"M\t{path}\n" for path in paths),
                encoding="utf-8",
            )
            env = os.environ.copy()
            env["GITHUB_OUTPUT"] = str(output)
            result = subprocess.run(
                [
                    sys.executable,
                    str(SCRIPTS_ROOT / script),
                    "--changed-files-from",
                    str(changed),
                ],
                cwd=REPO_ROOT,
                env=env,
                check=False,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(set(json.loads(result.stdout)[matrix_key]), expected)
            self.assertIn("unknown_paths=[]", output.read_text(encoding="utf-8"))

    def test_framework_only_inputs_route_through_public_selectors(self) -> None:
        arduino_only = (
            ".github/workflows/arduino-examples.yml",
            ".github/scripts/discover_arduino_examples.py",
            ".github/scripts/package_arduino_firmware.py",
            ".github/scripts/tests/test_package_arduino_firmware.py",
            ".github/scripts/tests/test_arduino_serial_readiness.py",
            "examples/arduino/common/serial_log.h",
        )
        idf_only = (
            ".github/workflows/esp-idf-examples.yml",
            ".github/scripts/discover_esp_idf_examples.py",
            "config/esp32p4_local_defaults.cmake",
            "config/esp32p4_rev1_3.defaults",
            "config/esp32p4_rev3_x.defaults",
        )
        cases = (
            ("discover_arduino_examples.py", "sketch", arduino_only, ARDUINO_SKETCHES),
            ("discover_esp_idf_examples.py", "example", arduino_only, set()),
            ("discover_esp_idf_examples.py", "example", idf_only, IDF_EXAMPLES),
            ("discover_arduino_examples.py", "sketch", idf_only, set()),
        )
        for script, matrix_key, paths, expected in cases:
            with self.subTest(script=script, paths=paths):
                self.assert_selector_paths(
                    script=script,
                    matrix_key=matrix_key,
                    paths=paths,
                    expected=expected,
                )

    def test_workflow_changed_file_invocation(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            temp = Path(temp_dir)
            changed = temp / "changed-files.txt"
            output = temp / "github-output.txt"
            changed.write_text(
                "M\texamples/esp-idf/02_HelloWorld/main/hello_world_main.c\n",
                encoding="utf-8",
            )
            env = os.environ.copy()
            env["GITHUB_OUTPUT"] = str(output)
            result = subprocess.run(
                [
                    sys.executable,
                    str(SCRIPTS_ROOT / "discover_esp_idf_examples.py"),
                    "--changed-files-from",
                    str(changed),
                ],
                cwd=REPO_ROOT,
                env=env,
                check=False,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(
                json.loads(result.stdout)["example"],
                ["examples/esp-idf/02_HelloWorld"],
            )
            self.assertIn("has_examples=true", output.read_text(encoding="utf-8"))

    def test_arduino_workflow_changed_file_invocation(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            temp = Path(temp_dir)
            changed = temp / "changed-files.txt"
            output = temp / "github-output.txt"
            changed.write_text(
                "M\texamples/arduino/examples/HelloWorld/HelloWorld.ino\n",
                encoding="utf-8",
            )
            env = os.environ.copy()
            env["GITHUB_OUTPUT"] = str(output)
            result = subprocess.run(
                [
                    sys.executable,
                    str(SCRIPTS_ROOT / "discover_arduino_examples.py"),
                    "--changed-files-from",
                    str(changed),
                ],
                cwd=REPO_ROOT,
                env=env,
                check=False,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(
                json.loads(result.stdout)["sketch"],
                ["examples/arduino/examples/HelloWorld"],
            )
            github_output = output.read_text(encoding="utf-8")
            self.assertIn("has_sketches=true", github_output)
            self.assertIn(
                "sketches=examples/arduino/examples/HelloWorld",
                github_output,
            )

    def test_base_ref_invocations_use_a_real_diverged_git_dag(self) -> None:
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
            idf_project = root / "examples" / "esp-idf" / "Demo"
            (idf_project / "main").mkdir(parents=True)
            (idf_project / "CMakeLists.txt").write_text(
                "cmake_minimum_required(VERSION 3.16)\n",
                encoding="utf-8",
            )
            (idf_project / "main" / "main.c").write_text(
                "void app_main(void) {}\n",
                encoding="utf-8",
            )
            sketch = root / "examples" / "arduino" / "examples" / "Blink"
            sketch.mkdir(parents=True)
            (sketch / "Blink.ino").write_text("void setup() {}\n", encoding="utf-8")
            (root / "README.md").write_text("# Initial\n", encoding="utf-8")
            git("add", ".")
            git("commit", "-m", "initial")

            git("switch", "-c", "feature")
            (idf_project / "main" / "main.c").write_text(
                "void app_main(void) { int changed = 1; (void) changed; }\n",
                encoding="utf-8",
            )
            (sketch / "Blink.ino").write_text(
                "void setup() { pinMode(1, OUTPUT); }\n",
                encoding="utf-8",
            )
            git("add", ".")
            git("commit", "-m", "feature")
            head_sha = git("rev-parse", "HEAD")

            git("switch", "main")
            (root / "README.md").write_text("# Base advanced\n", encoding="utf-8")
            git("add", "README.md")
            git("commit", "-m", "advance base")
            base_sha = git("rev-parse", "HEAD")

            commands = (
                (
                    "discover_esp_idf_examples.py",
                    "example",
                    ["examples/esp-idf/Demo"],
                ),
                (
                    "discover_arduino_examples.py",
                    "sketch",
                    ["examples/arduino/examples/Blink"],
                ),
            )
            for script, matrix_key, expected in commands:
                with self.subTest(script=script):
                    result = subprocess.run(
                        [
                            sys.executable,
                            str(SCRIPTS_ROOT / script),
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
                    self.assertEqual(json.loads(result.stdout)[matrix_key], expected)

    def test_missing_base_ref_is_an_operational_error(self) -> None:
        missing_base = "0" * 40
        for script in (
            "discover_esp_idf_examples.py",
            "discover_arduino_examples.py",
        ):
            with self.subTest(script=script):
                result = subprocess.run(
                    [
                        sys.executable,
                        str(SCRIPTS_ROOT / script),
                        "--base-ref",
                        missing_base,
                        "--head-ref",
                        "HEAD",
                    ],
                    cwd=REPO_ROOT,
                    check=False,
                    text=True,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                )
                self.assertEqual(result.returncode, 2)
                self.assertIn("Could not classify changed files", result.stderr)

    def test_empty_workflow_diff_returns_operational_error(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            changed = Path(temp_dir) / "changed-files.txt"
            changed.write_text("", encoding="utf-8")
            result = subprocess.run(
                [
                    sys.executable,
                    str(SCRIPTS_ROOT / "discover_arduino_examples.py"),
                    "--changed-files-from",
                    str(changed),
                ],
                cwd=REPO_ROOT,
                check=False,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            self.assertEqual(result.returncode, 2)
            self.assertIn("empty", result.stderr)


class ChangeRangeFetchTests(unittest.TestCase):
    def test_workflows_fetch_complete_change_range_history(self) -> None:
        workflows = (
            "esp-idf-examples.yml",
            "arduino-examples.yml",
            "product-firmware.yml",
            "repository-policy.yml",
        )
        for name in workflows:
            with self.subTest(workflow=name):
                text = (REPO_ROOT / ".github" / "workflows" / name).read_text(
                    encoding="utf-8"
                )
                self.assertIn("fetch-depth: 0", text)
                self.assertIn(
                    "PR_BASE_REF: ${{ github.event.pull_request.base.ref }}",
                    text,
                )
                self.assertIn(
                    'canonical_base="refs/remotes/canonical/${PR_BASE_REF}"',
                    text,
                )
                self.assertIn(
                    '"+refs/heads/${PR_BASE_REF}:${canonical_base}"',
                    text,
                )
                self.assertIn(
                    '[[ "$(git rev-parse --is-shallow-repository)" == "false" ]]',
                    text,
                )
                self.assertIn('git cat-file -e "${PR_BASE_SHA}^{commit}"', text)
                self.assertIn('git cat-file -e "${PR_HEAD_SHA}^{commit}"', text)
                self.assertIn(
                    'git merge-base "$PR_BASE_SHA" "$PR_HEAD_SHA" >/dev/null',
                    text,
                )
                self.assertIn(
                    'if ! git cat-file -e "${PUSH_BASE_SHA}^{commit}"; then',
                    text,
                )
                self.assertIn('git cat-file -e "${GITHUB_SHA}^{commit}"', text)
                self.assertNotIn("--depth=1", text)


class ArduinoWorkflowPackagingTests(unittest.TestCase):
    def test_pinned_segmented_packaging_contract(self) -> None:
        workflow = (
            REPO_ROOT / ".github" / "workflows" / "arduino-examples.yml"
        ).read_text(encoding="utf-8")

        pinned_values = (
            'ARDUINO_CORE_VERSION: "3.3.11"',
            'ARDUINO_ESPTOOL_VERSION: "5.3.1"',
            'ARDUINO_UPLOAD_SPEED: "921600"',
            'REGISTRY_BSP_COMPONENT: "waveshare/esp32_p4_platform"',
            'REGISTRY_BSP_VERSION: "2.0.1"',
            'REGISTRY_BSP_COMPONENT_HASH: "b1be590dd7170ea417a3b950b81c7892ff266d6514070ecdd568faeb654b0149"',
            'REGISTRY_BSP_REPOSITORY_COMMIT: "bfeb6e6d5737178cdde78b630c8118074da0a657"',
        )
        for value in pinned_values:
            with self.subTest(value=value):
                self.assertIn(value, workflow)

        self.assertIn(
            'ARDUINO_BOARD_OPTIONS: "PSRAM=enabled,USBMode=default,ChipVariant=postv3,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB"',
            workflow,
        )
        self.assertNotIn("ChipVariant=prev3", workflow)

        self.assertIn("arduino-cli board details", workflow)
        self.assertIn('mkdir -p "$(dirname "$build_path")" "$(dirname "$compile_log")"', workflow)
        self.assertIn('2>&1 | tee "$compile_log"', workflow)
        self.assertIn("--compile-log", workflow)
        self.assertIn("--show-properties=expanded", workflow)
        self.assertIn('source_sha="$(git rev-parse HEAD)"', workflow)
        self.assertIn('[[ "$source_sha" == "$EXPECTED_SOURCE_SHA" ]]', workflow)
        self.assertIn("python .github/scripts/package_arduino_firmware.py", workflow)
        for option in (
            "--build-dir",
            "--board-properties",
            "--output-dir",
            "--sketch-path",
            "--framework-version",
            "--arduino-cli-version",
            "--esptool-version",
            "--fqbn",
            "--board-options",
            "--git-sha",
            "--baud",
            "--bsp-component",
            "--bsp-version",
            "--bsp-component-hash",
            "--bsp-repository-commit",
        ):
            with self.subTest(option=option):
                self.assertIn(option, workflow)
        # Trusted-current-repo gate: the workflow must not hand the packager a
        # caller-chosen repository root; the packager derives it from __file__.
        self.assertNotIn("--repository-root", workflow)
        self.assertNotIn("--repo ", workflow)
        self.assertNotIn("--repo-root", workflow)
        # Compile-level path elimination covers the checkout, build directory,
        # Arduino data/tool cache, and broader home prefix before packaging
        # scans every binary member fail closed.
        for mapping in (
            "-ffile-prefix-map=${HOME}=host",
            "-fmacro-prefix-map=${HOME}=host",
            "-ffile-prefix-map=${GITHUB_WORKSPACE}=product",
            "-fmacro-prefix-map=${GITHUB_WORKSPACE}=product",
            "-ffile-prefix-map=${arduino_data_dir}=arduino-data",
            "-fmacro-prefix-map=${arduino_data_dir}=arduino-data",
            "-ffile-prefix-map=${build_path}=build",
            "-fmacro-prefix-map=${build_path}=build",
        ):
            self.assertIn(mapping, workflow)
        self.assertLess(
            workflow.index("-ffile-prefix-map=${HOME}=host"),
            workflow.index("-ffile-prefix-map=${arduino_data_dir}=arduino-data"),
        )
        for compiler in ("c", "cpp", "S"):
            self.assertIn(
                f'compiler.{compiler}.extra_flags=${{prefix_flags}}', workflow
            )

        upload_step = workflow.partition(
            "      - name: Upload segmented flash archive\n"
        )[2].partition("\n  result:")[0]
        self.assertTrue(upload_step, "missing segmented archive upload step")
        self.assertIn("uses: actions/upload-artifact@v4", upload_step)
        self.assertIn("${{ steps.package.outputs.manifest_path }}", upload_step)
        self.assertIn("${{ steps.package.outputs.archive_path }}", upload_step)
        self.assertNotIn("steps.package.outputs.package_dir", upload_step)
        self.assertIn("if-no-files-found: error", upload_step)
        self.assertIn("compression-level: 0", upload_step)
        self.assertNotIn("arduino-build", upload_step)
        self.assertNotIn(".merged.bin", upload_step)

        package_step = workflow.partition(
            "      - name: Package segmented flash archive\n"
        )[2].partition("\n      - name: Upload segmented flash archive")[0]
        self.assertIn('find "$package_dir" -mindepth 1', package_step)
        self.assertIn('! -path "$manifest_path"', package_step)
        self.assertIn('! -path "$archive_path"', package_step)
        self.assertIn('if [[ -n "$unexpected_path" ]]', package_step)
        self.assertIn('[[ "${#archives[@]}" -eq 1 ]]', package_step)


if __name__ == "__main__":
    unittest.main()
