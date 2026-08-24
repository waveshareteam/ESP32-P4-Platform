#!/usr/bin/env python3
"""Fail-closed tests for segmented Arduino release packaging."""

from __future__ import annotations

import argparse
import copy
import importlib.util
import io
import json
import os
import shlex
import shutil
import stat
import struct
import subprocess
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path

# Keep the clean checkout clean: importing the packager must not write a
# __pycache__ .pyc into the trusted repository working tree.
sys.dont_write_bytecode = True


REPO_ROOT = Path(__file__).resolve().parents[3]
SCRIPTS_ROOT = REPO_ROOT / ".github" / "scripts"
sys.path.insert(0, str(SCRIPTS_ROOT))

# Loaded fresh from a clean checkout in setUp so the packager anchors its
# trusted repository to its own __file__ location (the trusted-current-repo gate),
# never to a caller-supplied --repository-root.
packager = None

FQBN = "esp32:esp32:esp32p4"
BOARD_OPTIONS = (
    "PSRAM=enabled,USBMode=default,ChipVariant=prev3,FlashSize=16M,"
    "PartitionScheme=app3M_fat9M_16MB"
)
BSP_ARGS = {
    "bsp_component": "waveshare/esp32_p4_platform",
    "bsp_version": "2.0.1",
    "bsp_component_hash": (
        "b1be590dd7170ea417a3b950b81c7892ff266d6514070ecdd568faeb654b0149"
    ),
    "bsp_repository_commit": "bfeb6e6d5737178cdde78b630c8118074da0a657",
}


class ArduinoPackageTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory(prefix="arduino-packager-test-")
        self.root = Path(self.temporary.name)
        self.checkout = self.root / "checkout"
        self.sketch_relative = Path("examples/arduino/examples/Demo")
        self.sketch_dir = self.checkout / self.sketch_relative
        self.build_dir = self.root / "build"
        self.tool_dir = self.root / "tool"
        self.platform_dir = self.root / "arduino-core" / "3.3.11"
        self.properties_path = self.root / "board.properties.txt"
        self.compile_log_path = self.root / "compile.verbose.log"
        self.output_index = 0
        self.default_segments = [
            (0x2000, "Demo.ino.bootloader.bin", b"B" * 0x1000),
            (0x8000, "Demo.ino.partitions.bin", b"P" * 0x1000),
            (0xE000, "boot_app0.bin", b"O" * 0x1000),
            (0x10000, "Demo.ino.bin", b"A" * 0x2000),
        ]
        self._make_checkout()
        self._load_packager()
        self.write_fixture(self.default_segments)

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def _git(self, *arguments: str) -> str:
        result = subprocess.run(
            ["git", "-C", str(self.checkout), *arguments],
            capture_output=True,
            text=True,
            check=True,
        )
        return result.stdout.strip()

    def _make_checkout(self) -> None:
        scripts_dir = self.checkout / ".github" / "scripts"
        scripts_dir.mkdir(parents=True)
        shutil.copy(
            SCRIPTS_ROOT / "package_arduino_firmware.py",
            scripts_dir / "package_arduino_firmware.py",
        )
        self.sketch_dir.mkdir(parents=True)
        (self.sketch_dir / "Demo.ino").write_text(
            "void setup() {}\nvoid loop() {}\n", encoding="utf-8"
        )
        # A second, differently-sourced sketch of a different name stays
        # committed so swap/spoof negative tests do not dirty the checkout.
        other_dir = self.checkout / "examples" / "arduino" / "examples" / "Other"
        other_dir.mkdir(parents=True)
        (other_dir / "Other.ino").write_text(
            "void setup() { int other = 1; (void)other; }\nvoid loop() {}\n",
            encoding="utf-8",
        )
        subprocess.run(["git", "init", "-q"], cwd=self.checkout, check=True)
        subprocess.run(
            ["git", "-c", "user.email=a@b", "-c", "user.name=t", "add", "-A"],
            cwd=self.checkout,
            check=True,
        )
        subprocess.run(
            [
                "git",
                "-c",
                "user.email=a@b",
                "-c",
                "user.name=t",
                "commit",
                "-q",
                "-m",
                "fixture",
            ],
            cwd=self.checkout,
            check=True,
        )
        self.head = self._git("rev-parse", "HEAD")
        self.assertEqual(self._git("status", "--porcelain=v1", "--untracked-files=all"), "")

    def _load_packager(self) -> None:
        global packager
        spec = importlib.util.spec_from_file_location(
            "package_arduino_firmware_under_test",
            self.checkout / ".github" / "scripts" / "package_arduino_firmware.py",
        )
        assert spec is not None and spec.loader is not None
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        spec.loader.exec_module(module)
        packager = module

    def write_fixture(self, segments: list[tuple[int, str, bytes]]) -> None:
        shutil.rmtree(self.build_dir, ignore_errors=True)
        shutil.rmtree(self.tool_dir, ignore_errors=True)
        shutil.rmtree(self.platform_dir.parent, ignore_errors=True)
        self.build_dir.mkdir(parents=True)
        self.tool_dir.mkdir(parents=True)
        (self.platform_dir / "tools" / "partitions").mkdir(parents=True)
        (self.platform_dir / "platform.txt").write_text(
            "name=Fixture Arduino core\nversion=3.3.11\n", encoding="utf-8"
        )
        self.sketch_dir.mkdir(parents=True, exist_ok=True)
        (self.sketch_dir / "Demo.ino").write_text(
            "void setup() {}\nvoid loop() {}\n", encoding="utf-8"
        )
        for _offset, name, data in segments:
            path = self.build_dir / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)

        segment_lines = [f"{offset:#x} {name}" for offset, name, _data in segments]
        (self.build_dir / "flash_args").write_text(
            "--flash-mode dio --flash-freq 80m --flash-size 16MB\n"
            + "\n".join(segment_lines)
            + "\n",
            encoding="utf-8",
        )
        (self.build_dir / "build.options.json").write_text(
            json.dumps(
                {
                    "fqbn": f"{FQBN}:{BOARD_OPTIONS}",
                    "sketchLocation": self.sketch_dir.resolve().as_posix(),
                }
            )
            + "\n",
            encoding="utf-8",
        )
        (self.build_dir / "sdkconfig").write_text(
            'CONFIG_IDF_TARGET="esp32p4"\n', encoding="utf-8"
        )
        generated_dir = self.build_dir / "sketch"
        generated_dir.mkdir()
        generated_source = generated_dir / "Demo.ino.cpp"
        primary_abs = (self.sketch_dir / "Demo.ino").resolve().as_posix()
        generated_source.write_text(
            "#include <Arduino.h>\n"
            f'#line 1 "{primary_abs}"\n'
            f'#line 1 "{primary_abs}"\n'
            'void setup();\n'
            f'#line 2 "{primary_abs}"\n'
            'void loop();\n'
            f'#line 1 "{primary_abs}"\n'
            'void setup() {}\n'
            'void loop() {}\n',
            encoding="utf-8",
        )
        generated_object = generated_dir / "Demo.ino.cpp.o"
        generated_object.write_bytes(b"compiled-object")
        application_elf = self.build_dir / "Demo.ino.elf"
        application_elf.write_bytes(b"linked-elf")
        core_archive = self.build_dir / "core" / "core.a"
        core_archive.parent.mkdir()
        core_archive.write_bytes(b"fixture-core-archive")
        link_map = self.build_dir / "Demo.ino.map"
        link_map.write_text(
            f"OUTPUT({application_elf.as_posix()} elf32-littleriscv)\n"
            f"LOAD {generated_object.as_posix()}\n",
            encoding="utf-8",
        )
        compiler_dir = self.root / "toolchain" / "bin"
        compiler_dir.mkdir(parents=True, exist_ok=True)
        fake_compiler = compiler_dir / "riscv32-esp-elf-g++"
        trusted_cwd = shlex.quote(self.checkout.resolve().as_posix())
        fake_compiler.write_text(
            "#!/bin/sh\n"
            f"expected_cwd={trusted_cwd}\n"
            '[ "$(pwd -P)" = "$expected_cwd" ] || exit 97\n'
            "out= map= obj= mode=\n"
            "previous=\n"
            "for value in \"$@\"; do\n"
            "  [ \"$previous\" = -o ] && out=$value\n"
            "  [ \"$value\" = -c ] && mode=compile\n"
            "  case $value in -Wl,--Map=*) map=${value#-Wl,--Map=};; *.o) obj=$value;; esac\n"
            "  previous=$value\n"
            "done\n"
            "if [ \"$mode\" = compile ]; then printf compiled-object > \"$out\"; "
            "else printf linked-elf > \"$out\"; printf 'OUTPUT(%s elf32-littleriscv)\\nLOAD %s\\n' \"$out\" \"$obj\" > \"$map\"; fi\n",
            encoding="utf-8",
        )
        fake_compiler.chmod(0o755)
        (self.build_dir / "compile_commands.json").write_text(
            json.dumps(
                [
                    {
                        "directory": self.checkout.resolve().as_posix(),
                        "file": generated_source.resolve().as_posix(),
                        "command": (
                            fake_compiler.as_posix()
                            + " -c "
                            + generated_source.resolve().as_posix()
                            + " -o "
                            + generated_object.resolve().as_posix()
                        ),
                    }
                ]
            )
            + "\n",
            encoding="utf-8",
        )

        fake_esptool = self.tool_dir / "esptool"
        fake_esptool.write_text(
            "#!/bin/sh\n"
            "if [ \"${1:-}\" = version ]; then\n"
            "  printf 'esptool v5.3.1\\n5.3.1\\n'\n"
            "  exit 0\n"
            "fi\n"
            f"expected_cwd={trusted_cwd}\n"
            '[ "$(pwd -P)" = "$expected_cwd" ] || exit 97\n'
            "if [ \"${3:-}\" = image-info ]; then\n"
            "  app=${4:-}; elf=$(dirname \"$app\")/Demo.ino.elf\n"
            "  printf 'ELF file SHA256: %s\\n' \"$(sha256sum \"$elf\" | awk '{print $1}')\"\n"
            "  printf 'Checksum: 00 (valid)\\nValidation hash: 00 (valid)\\n'\n"
            "  exit 0\n"
            "fi\n"
            "if [ \"${3:-}\" = elf2image ]; then\n"
            "  out=; previous=; for value in \"$@\"; do [ \"$previous\" = -o ] && out=$value; previous=$value; done\n"
            "  head -c 8192 /dev/zero | tr '\\000' A > \"$out\"\n"
            "  exit 0\n"
            "fi\n"
            "exit 88\n",
            encoding="utf-8",
        )
        fake_esptool.chmod(0o755)
        offsets_and_names_list: list[str] = []
        for offset, name, data in segments:
            if name == "boot_app0.bin":
                core_source = self.platform_dir / "tools" / "partitions" / name
                core_source.write_bytes(data)
                recipe_source = core_source.as_posix()
            else:
                recipe_source = f"{{build.path}}/{name}"
            offsets_and_names_list.append(f'{offset:#x} "{recipe_source}"')
        offsets_and_names = " ".join(offsets_and_names_list)
        upload_recipe = (
            "--chip esp32p4 --port {serial.port} --baud 921600 "
            "--before default-reset --after hard-reset write-flash -z "
            "--flash-mode keep --flash-freq keep --flash-size keep "
            + offsets_and_names
        )
        properties = {
            "_id": "esp32p4",
            "version": "3.3.11",
            "build.mcu": "esp32p4",
            "build.chip_variant": "esp32p4_es",
            "build.flash_mode": "dio",
            "build.flash_freq": "80m",
            "build.flash_size": "16MB",
            "build.partitions": "app3M_fat9M_16MB",
            "build.usb_mode": "0",
            "build.cdc_on_boot": "0",
            "build.msc_on_boot": "0",
            "build.dfu_on_boot": "0",
            "upload.speed": "921600",
            "upload.use_1200bps_touch": "false",
            "upload.wait_for_upload_port": "false",
            "runtime.platform.path": self.platform_dir.as_posix(),
            "tools.esptool_py.path": self.tool_dir.as_posix(),
            "tools.esptool_py.cmd": "esptool",
            "tools.esptool_py.upload.pattern_args": upload_recipe,
            "runtime.tools.esp-rv32.path": compiler_dir.parent.as_posix(),
            "compiler.cpp.cmd": "riscv32-esp-elf-g++",
            "recipe.c.combine.pattern": (
                f'\"{fake_compiler.as_posix()}\" -Wl,--Map={{build.path}}/{{build.project_name}}.map '
                "-Wl,--start-group {object_files} {archive_file_path} -Wl,--end-group -o "
                '\"{build.path}/{build.project_name}.elf\"'
            ),
            "recipe.objcopy.bin.pattern_args": (
                '--chip esp32p4 elf2image -o "{build.path}/{build.project_name}.bin" '
                '"{build.path}/{build.project_name}.elf"'
            ),
        }
        self.properties_path.write_text(
            "".join(f"{key}={value}\n" for key, value in properties.items()),
            encoding="utf-8",
        )
        self.compile_log_path.write_text(
            "\n".join(
                (
                    f"{fake_compiler.as_posix()} -c {generated_source.as_posix()} -o {generated_object.as_posix()}",
                    f"{fake_compiler.as_posix()} -Wl,--Map={link_map.as_posix()} -Wl,--start-group {generated_object.as_posix()} {core_archive.as_posix()} -Wl,--end-group -o {application_elf.as_posix()}",
                    f"{fake_esptool.as_posix()} --chip esp32p4 elf2image -o {(self.build_dir / 'Demo.ino.bin').as_posix()} {application_elf.as_posix()}",
                )
            ) + "\n",
            encoding="utf-8",
        )

    def arguments(self, **overrides: object) -> argparse.Namespace:
        self.output_index += 1
        values: dict[str, object] = {
            "build_dir": self.build_dir,
            "board_properties": self.properties_path,
            "compile_log": self.compile_log_path,
            "output_dir": self.root / f"output-{self.output_index}",
            "sketch_path": "examples/arduino/examples/Demo",
            "framework_version": "3.3.11",
            "arduino_cli_version": "1.5.1",
            "esptool_version": "5.3.1",
            "fqbn": FQBN,
            "board_options": BOARD_OPTIONS,
            "git_sha": self.head,
            "timestamp_utc": "2026-08-15T00:00:00Z",
            "baud": 921600,
            **BSP_ARGS,
        }
        values.update(overrides)
        return argparse.Namespace(**values)

    def local_evidence(self) -> packager.LocalBuildEvidence:
        return packager.LocalBuildEvidence(
            build_dir=self.build_dir,
            board_properties=self.properties_path,
            compile_log=self.compile_log_path,
        )

    def validate_archive(
        self,
        archive_path: Path,
        *,
        expected_manifest: bytes | None = None,
        local_build: object | None = None,
    ) -> dict[str, object]:
        evidence = self.local_evidence() if local_build is None else local_build
        return packager.validate_archive(
            archive_path,
            local_build=evidence,
            expected_manifest=expected_manifest,
        )

    def package(self, **overrides: object) -> tuple[Path, Path]:
        return packager.package(self.arguments(**overrides))

    @staticmethod
    def read_members(archive_path: Path) -> dict[str, bytes]:
        with zipfile.ZipFile(archive_path) as archive:
            return {name: archive.read(name) for name in archive.namelist()}

    def write_mutated_archive(self, members: dict[str, bytes], name: str) -> Path:
        members.pop(packager.CHECKSUMS_SELF_PATH, None)
        packager.validate_public_text_members(members)
        members[packager.CHECKSUMS_SELF_PATH] = packager.checksum_document(members)
        path = self.root / name
        packager.write_zip(path, members)
        return path

    def assert_validator_rejects(self, archive_path: Path, message: str) -> None:
        with self.assertRaisesRegex(packager.PackagingError, message):
            self.validate_archive(archive_path)

        environment = os.environ.copy()
        environment["PYTHONPATH"] = str(self.checkout / ".github" / "scripts")
        result = subprocess.run(
            [
                sys.executable,
                "-B",
                "-c",
                (
                    "from pathlib import Path; "
                    "import package_arduino_firmware as package; "
                    "import sys; "
                    "package.validate_archive(Path(sys.argv[1]), "
                    "local_build=package.LocalBuildEvidence("
                    "Path(sys.argv[2]), Path(sys.argv[3]), Path(sys.argv[4])))"
                ),
                str(archive_path),
                str(self.build_dir),
                str(self.properties_path),
                str(self.compile_log_path),
            ],
            env=environment,
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        self.assertNotEqual(
            result.returncode,
            0,
            "tampered ZIP validator process unexpectedly returned success",
        )
        self.assertRegex(result.stdout + result.stderr, message)

    def assert_validator_subprocess_accepts(self, archive_path: Path) -> None:
        environment = os.environ.copy()
        environment["PYTHONPATH"] = str(self.checkout / ".github" / "scripts")
        result = subprocess.run(
            [
                sys.executable, "-B", "-c",
                (
                    "from pathlib import Path; import package_arduino_firmware as package; import sys; "
                    "package.validate_archive(Path(sys.argv[1]), local_build=package.LocalBuildEvidence("
                    "Path(sys.argv[2]), Path(sys.argv[3]), Path(sys.argv[4])))"
                ),
                str(archive_path), str(self.build_dir), str(self.properties_path), str(self.compile_log_path),
            ],
            env=environment, check=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    @staticmethod
    def encode_manifest(document: dict[str, object]) -> bytes:
        return (json.dumps(document, indent=2, sort_keys=True) + "\n").encode()

    def test_realistic_segmented_package_is_self_verifying(self) -> None:
        archive_path, manifest_path = self.package()
        manifest = self.validate_archive(
            archive_path,
            expected_manifest=manifest_path.read_bytes(),
            local_build=self.local_evidence(),
        )
        members = self.read_members(archive_path)
        self.assertEqual(manifest["product_git_sha"], self.head)
        self.assertEqual(manifest["bsp"], packager.expected_bsp_record())
        self.assertEqual(manifest["flash"]["segmented_payload_total"], 0x5000)
        self.assertEqual(
            [item["role"] for item in manifest["flash"]["segments"]],
            ["bootloader", "partition_table", "boot_app0", "application"],
        )
        command = manifest["flash"]["write_flash_command"]
        self.assertIn("--baud 921600", command)
        self.assertIn("default-reset", command)
        self.assertIn("hard-reset write-flash", command)
        self.assertNotIn("write_flash", command)
        self.assertNotIn("merged", "\n".join(members).lower())
        self.assertEqual(members["requirements.txt"], b"esptool==5.3.1\n")

        checksums = packager.parse_checksums(members["SHA256SUMS.txt"])
        self.assertEqual(set(checksums), set(members) - {"SHA256SUMS.txt"})
        for name, digest in checksums.items():
            self.assertEqual(digest, packager.sha256_bytes(members[name]))
        for _offset, source_name, expected_bytes in self.default_segments:
            self.assertEqual(members[f"bin/{source_name}"], expected_bytes)

        build = manifest["build"]
        identity = json.loads(members["metadata/build_identity.json"])
        self.assertEqual(
            identity["build"],
            {key: value for key, value in build.items() if key != "identity_metadata_path"},
        )
        self.assertEqual(
            build["raw_evidence"]["build_options"]["basename"], "build.options.json"
        )
        self.assertEqual(
            build["raw_evidence"]["board_properties"]["basename"],
            self.properties_path.name,
        )
        self.assertEqual(
            build["raw_evidence"]["compile_commands"]["basename"],
            "compile_commands.json",
        )
        self.assertEqual(build["sketch_name"], "Demo")
        self.assertEqual(build["primary_source_basename"], "Demo.ino")
        self.assertEqual(
            build["generated_translation_unit_basename"], "Demo.ino.cpp"
        )
        self.assertEqual(build["application_binary_basename"], "Demo.ino.bin")
        self.assertEqual(
            manifest["flash"]["recipe_provenance"]["segments"],
            [
                {"offset": offset, "source_name": source_name}
                for offset, source_name, _data in self.default_segments
            ],
        )
        self.assertEqual(build["resolved_properties"]["build.cdc_on_boot"], "0")
        self.assertEqual(build["resolved_properties"]["build.usb_mode"], "0")
        self.assertNotIn("build.options.json", members)
        self.assertNotIn("compile_commands.json", members)
        self.assertNotIn("Demo.ino", members)
        self.assertNotIn(self.properties_path.name, members)
        for name in packager.PUBLIC_TEXT_MEMBERS:
            if name in members:
                text = members[name].decode()
                self.assertNotIn(self.root.as_posix(), text)
                self.assertNotIn("/home/", text)
                self.assertNotIn("/tmp/", text)

        shell_path = self.root / "flash.sh"
        shell_path.write_bytes(members["flash.sh"])
        shell_path.chmod(0o755)
        syntax = subprocess.run(["sh", "-n", str(shell_path)], capture_output=True, text=True)
        self.assertEqual(syntax.returncode, 0, syntax.stderr)
        shell_text = members["flash.sh"].decode()
        cmd_text = members["flash.cmd"].decode()
        self.assertIn('FLASH_PORT="${1:-${PORT:-}}"', shell_text)
        self.assertIn('set "FLASH_PORT=%~1"', cmd_text)
        self.assertIn("python -m esptool", shell_text)
        self.assertIn("write-flash", shell_text)
        self.assertNotIn("write_flash", shell_text + cmd_text)

    def test_source_to_binary_validation_requires_actual_local_build_evidence(self) -> None:
        archive, _manifest = self.package()
        with self.assertRaisesRegex(packager.PackagingError, "local build evidence"):
            packager.validate_archive(archive, local_build=None)

    def test_legitimate_bootloader_at_zero_is_allowed(self) -> None:
        segments = copy.deepcopy(self.default_segments)
        segments[0] = (0, segments[0][1], segments[0][2])
        self.write_fixture(segments)
        archive, _manifest = self.package()
        validated = self.validate_archive(
            archive, local_build=self.local_evidence()
        )
        self.assertEqual(validated["flash"]["segments"][0]["offset"], 0)

    def test_build_rejects_overlap_and_out_of_bounds(self) -> None:
        cases = {
            "overlap": [
                (0x2000, "Demo.ino.bootloader.bin", b"B" * 0x7000),
                *self.default_segments[1:],
            ],
            "out-of-bounds": [
                *self.default_segments[:-1],
                (0xFFFF00, "Demo.ino.bin", b"A" * 0x2000),
            ],
        }
        for label, segments in cases.items():
            with self.subTest(label=label):
                self.write_fixture(segments)
                with self.assertRaises(packager.PackagingError):
                    self.package()

    def test_segmented_payload_must_be_strictly_smaller_than_capacity(self) -> None:
        capacity = 16 * 1024 * 1024
        full = [
            (0x0, "Demo.ino.bootloader.bin", b"B" * 0x2000),
            (0x2000, "Demo.ino.partitions.bin", b"P" * 0x2000),
            (0x4000, "boot_app0.bin", b"O" * 0x2000),
            (0x6000, "Demo.ino.bin", b"A" * (capacity - 0x6000)),
        ]
        self.write_fixture(full)
        with self.assertRaisesRegex(packager.PackagingError, "payload total.*smaller"):
            self.package()

    def test_rejects_16_mib_merged_image(self) -> None:
        segments = copy.deepcopy(self.default_segments)
        segments[-1] = (0x10000, "Demo.ino.merged.bin", b"M" * (16 * 1024 * 1024))
        self.write_fixture(segments)
        with self.assertRaisesRegex(packager.PackagingError, "merged/whole-flash|application binary"):
            self.package()

    def test_recipe_offsets_are_actual_provenance(self) -> None:
        text = self.properties_path.read_text(encoding="utf-8")
        self.properties_path.write_text(text.replace("0x8000", "0x9000"), encoding="utf-8")
        with self.assertRaisesRegex(
            packager.PackagingError, "(offset/file pairs differ|flash_args build file)"
        ):
            self.package()

    def test_recipe_offset_and_filename_pairs_are_actual_provenance(self) -> None:
        wrong_name = "Wrong.ino.partitions.bin"
        (self.build_dir / wrong_name).write_bytes(
            (self.build_dir / "Demo.ino.partitions.bin").read_bytes()
        )
        text = self.properties_path.read_text(encoding="utf-8")
        self.properties_path.write_text(
            text.replace("Demo.ino.partitions.bin", wrong_name, 1), encoding="utf-8"
        )
        with self.assertRaisesRegex(
            packager.PackagingError, "(offset/file pairs differ|flash_args build file)"
        ):
            self.package()

    def test_recipe_segment_must_be_the_exact_build_directory_file(self) -> None:
        outside = self.root / "outside" / "Demo.ino.partitions.bin"
        outside.parent.mkdir()
        outside.write_bytes((self.build_dir / outside.name).read_bytes())
        text = self.properties_path.read_text(encoding="utf-8")
        self.properties_path.write_text(
            text.replace(
                '"{build.path}/Demo.ino.partitions.bin"',
                f'"{outside.as_posix()}"',
                1,
            ),
            encoding="utf-8",
        )
        with self.assertRaisesRegex(packager.PackagingError, "exact build directory"):
            self.package()

    def test_only_exact_core_boot_app0_may_be_an_external_recipe_source(self) -> None:
        # The real ESP32 core upload recipe references its installed boot_app0,
        # while flash_args references the byte-identical build-directory copy.
        self.package()

        self.write_fixture(self.default_segments)
        arbitrary = self.root / "outside" / "boot_app0.bin"
        arbitrary.parent.mkdir()
        arbitrary.write_bytes((self.build_dir / "boot_app0.bin").read_bytes())
        text = self.properties_path.read_text(encoding="utf-8")
        core_source = self.platform_dir / "tools" / "partitions" / "boot_app0.bin"
        self.properties_path.write_text(
            text.replace(core_source.as_posix(), arbitrary.as_posix(), 1),
            encoding="utf-8",
        )
        with self.assertRaisesRegex(packager.PackagingError, "exact build directory"):
            self.package()

        self.write_fixture(self.default_segments)
        core_source.write_bytes(b"different-core-bytes")
        with self.assertRaisesRegex(packager.PackagingError, "bytes differ"):
            self.package()

        self.write_fixture(self.default_segments)
        core_source.unlink()
        arbitrary = self.root / "other-boot_app0.bin"
        arbitrary.write_bytes((self.build_dir / "boot_app0.bin").read_bytes())
        core_source.symlink_to(arbitrary)
        with self.assertRaisesRegex(packager.PackagingError, "missing or not a regular file"):
            self.package()

        self.write_fixture(self.default_segments)
        partitions = self.platform_dir / "tools" / "partitions"
        boot = partitions / "boot_app0.bin"
        boot.unlink()
        outside_dir = self.root / "outside-partitions"
        outside_dir.mkdir()
        (outside_dir / "boot_app0.bin").write_bytes((self.build_dir / "boot_app0.bin").read_bytes())
        partitions.rmdir()
        partitions.symlink_to(outside_dir, target_is_directory=True)
        with self.assertRaisesRegex(packager.PackagingError, "boot_app0 path contains a symlink"):
            self.package()

    def test_declared_sketch_and_build_directory_identity_cannot_be_swapped(self) -> None:
        other_relative = Path("examples/arduino/examples/Other")
        other_dir = self.checkout / other_relative
        cases = (
            (other_relative.as_posix(), self.sketch_dir),
            (self.sketch_relative.as_posix(), other_dir),
        )
        for declared_path, built_path in cases:
            with self.subTest(declared_path=declared_path, built_path=built_path.name):
                options_path = self.build_dir / "build.options.json"
                options = json.loads(options_path.read_text(encoding="utf-8"))
                options["sketchLocation"] = built_path.resolve().as_posix()
                options_path.write_text(json.dumps(options) + "\n", encoding="utf-8")
                with self.assertRaisesRegex(packager.PackagingError, "sketchLocation"):
                    self.package(sketch_path=declared_path)

    def test_compile_identity_and_application_basename_fail_closed(self) -> None:
        commands_path = self.build_dir / "compile_commands.json"
        commands = json.loads(commands_path.read_text(encoding="utf-8"))
        commands.append(copy.deepcopy(commands[0]))
        commands_path.write_text(json.dumps(commands) + "\n", encoding="utf-8")
        with self.assertRaisesRegex(packager.PackagingError, "exactly one entry"):
            self.package()

        wrong_segments = copy.deepcopy(self.default_segments)
        wrong_segments[-1] = (0x10000, "Other.ino.bin", wrong_segments[-1][2])
        self.write_fixture(wrong_segments)
        with self.assertRaisesRegex(packager.PackagingError, "application binary|missing roles: application"):
            self.package()

    def test_compile_command_must_compile_the_declared_generated_tu(self) -> None:
        other_generated = self.root / "Other.ino.cpp"
        other_generated.write_text("void setup() {}\nvoid loop() {}\n", encoding="utf-8")
        commands_path = self.build_dir / "compile_commands.json"
        commands = json.loads(commands_path.read_text(encoding="utf-8"))
        commands[0]["command"] = (
            f"fake-c++ -c {other_generated.as_posix()} -o "
            f"{(self.build_dir / 'sketch' / 'Demo.ino.cpp.o').as_posix()}"
        )
        commands_path.write_text(json.dumps(commands) + "\n", encoding="utf-8")
        with self.assertRaisesRegex(
            packager.PackagingError, "does not compile the expected generated sketch"
        ):
            self.package()

    def test_compile_log_and_link_inputs_are_exactly_trusted(self) -> None:
        original = self.compile_log_path.read_text(encoding="utf-8")
        first, rest = original.split("\n", 1)
        self.compile_log_path.write_text(first + " -DATTACK\n" + rest, encoding="utf-8")
        with self.assertRaisesRegex(packager.PackagingError, "differs from compile_commands"):
            self.package()

        self.write_fixture(self.default_segments)
        outside = self.root / "outside-object.o"
        outside.write_bytes(b"outside")
        line = self.compile_log_path.read_text(encoding="utf-8")
        # Replace only the linker occurrence; retain the compile record first.
        first, rest = line.split("\n", 1)
        rest = rest.replace((self.build_dir / "sketch" / "Demo.ino.cpp.o").as_posix(), outside.as_posix(), 1)
        self.compile_log_path.write_text(first + "\n" + rest, encoding="utf-8")
        with self.assertRaisesRegex(packager.PackagingError, "object input escapes"):
            self.package()

    def test_replay_uses_trusted_repository_cwd(self) -> None:
        unrelated = self.root / "unrelated-cwd"
        unrelated.mkdir()
        original_cwd = Path.cwd()
        original_replay = packager._run_replay
        try:
            os.chdir(unrelated)
            # This is the pre-fix behavior: replay inherits an unrelated
            # caller directory.  The fixture tools require the trusted root,
            # so this must fail before proving the hardened path succeeds.
            def caller_cwd_replay(
                tokens: list[str], label: str, repository_root: Path
            ) -> None:
                result = subprocess.run(
                    tokens, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                    check=False, timeout=180,
                )
                if result.returncode:
                    raise packager.PackagingError(f"{label} replay failed with {result.returncode}")

            packager._run_replay = caller_cwd_replay
            with self.assertRaisesRegex(packager.PackagingError, "replay failed with 97"):
                self.package()
            packager._run_replay = original_replay
            archive, manifest = self.package()
            self.validate_archive(archive, expected_manifest=manifest.read_bytes())
        finally:
            packager._run_replay = original_replay
            os.chdir(original_cwd)

    def _set_linker_expansion_controls(self, before: str, after: str) -> None:
        object_path = self.build_dir / "sketch" / "Demo.ino.cpp.o"
        core_path = self.build_dir / "core" / "core.a"
        compile_log = self.compile_log_path.read_text(encoding="utf-8")
        compile_log = compile_log.replace(
            f"-Wl,--start-group {object_path.as_posix()} {core_path.as_posix()}",
            f"-Wl,--start-group {before} {object_path.as_posix()} {after} {core_path.as_posix()}",
        )
        self.compile_log_path.write_text(compile_log, encoding="utf-8")

    def _set_linker_recipe_controls(self, before: str, after: str) -> None:
        object_path = self.build_dir / "sketch" / "Demo.ino.cpp.o"
        core_path = self.build_dir / "core" / "core.a"
        properties = self.properties_path.read_text(encoding="utf-8")
        properties = properties.replace(
            "-Wl,--start-group {object_files} {archive_file_path} -Wl,--end-group",
            f"{before} -Wl,--start-group {{object_files}} {{archive_file_path}} {after} -Wl,--end-group",
        )
        self.properties_path.write_text(properties, encoding="utf-8")
        compile_log = self.compile_log_path.read_text(encoding="utf-8")
        compile_log = compile_log.replace(
            f"-Wl,--start-group {object_path.as_posix()} {core_path.as_posix()} -Wl,--end-group",
            f"{before} -Wl,--start-group {object_path.as_posix()} {core_path.as_posix()} {after} -Wl,--end-group",
        )
        self.compile_log_path.write_text(compile_log, encoding="utf-8")

    def test_linker_object_files_expansion_allows_only_exact_whole_archive_pair(self) -> None:
        # The standard expansion has no controls.
        self.package()

        self.write_fixture(self.default_segments)
        self._set_linker_expansion_controls(
            "-Wl,--whole-archive", "-Wl,--no-whole-archive"
        )
        # This mirrors the real LVGL Arduino 3.3.11 link line while keeping
        # recipe.c.combine.pattern unchanged.
        self.package()

        cases = (
            ("arbitrary", "-Wl,--as-needed", "", "unexpected linker control"),
            ("missing-close", "-Wl,--whole-archive", "", "exact balanced pair"),
            ("missing-open", "", "-Wl,--no-whole-archive", "exact balanced pair"),
            (
                "reversed",
                "-Wl,--no-whole-archive",
                "-Wl,--whole-archive",
                "unexpected linker control",
            ),
            (
                "unbalanced",
                "-Wl,--whole-archive",
                "-Wl,--no-whole-archive -Wl,--no-whole-archive",
                "unexpected linker control",
            ),
            (
                "internal-control",
                "-Wl,--whole-archive -Wl,--as-needed",
                "-Wl,--no-whole-archive",
                "unexpected linker control",
            ),
            (
                "gnu-single-control",
                "-Wl,-whole-archive",
                "-Wl,-no-whole-archive",
                "unexpected linker control",
            ),
            (
                "abbreviated-control",
                "-Wl,--wh",
                "-Wl,--no-wh",
                "unexpected linker control",
            ),
        )
        for label, before, after, error in cases:
            with self.subTest(label=label):
                self.write_fixture(self.default_segments)
                self._set_linker_expansion_controls(before, after)
                with self.assertRaisesRegex(packager.PackagingError, error):
                    self.package()

        self.write_fixture(self.default_segments)
        self._set_linker_expansion_controls(
            "-Wl,--whole-archive", "-Wl,--no-whole-archive"
        )
        outside = self.root / "outside-whole-archive-object.o"
        outside.write_bytes(b"outside")
        compile_log = self.compile_log_path.read_text(encoding="utf-8")
        first, rest = compile_log.split("\n", 1)
        rest = rest.replace(
            (self.build_dir / "sketch" / "Demo.ino.cpp.o").as_posix(),
            outside.as_posix(),
            1,
        )
        self.compile_log_path.write_text(first + "\n" + rest, encoding="utf-8")
        with self.assertRaisesRegex(packager.PackagingError, "object input escapes"):
            self.package()

    def test_linker_recipe_rejects_whole_archive_controls_outside_expansion(self) -> None:
        cases = (
            ("prefix", "-Wl,--whole-archive", ""),
            ("suffix", "", "-Wl,--no-whole-archive"),
            (
                "repeated",
                "-Wl,--whole-archive -Wl,--whole-archive",
                "-Wl,--no-whole-archive -Wl,--no-whole-archive",
            ),
            ("combined-prefix", "-Wl,--whole-archive,--gc-sections", ""),
            ("combined-suffix", "", "-Wl,--no-whole-archive,--cref"),
            ("gnu-single-wl", "-Wl,-whole-archive", "-Wl,-no-whole-archive"),
            ("abbreviated-wl", "-Wl,--wh", "-Wl,--no-wh"),
            (
                "combined-repeated",
                "-Wl,--gc-sections,--whole-archive",
                "-Wl,--cref,--no-whole-archive",
            ),
            ("bare-prefix", "--whole-archive", ""),
            ("bare-suffix", "", "--no-whole-archive"),
            ("xlinker-prefix", "-Xlinker --whole-archive", ""),
            ("xlinker-suffix", "", "-Xlinker --no-whole-archive"),
            (
                "xlinker-repeated",
                "-Xlinker --whole-archive -Xlinker --whole-archive",
                "-Xlinker --no-whole-archive -Xlinker --no-whole-archive",
            ),
            (
                "xlinker-equals",
                "-Xlinker=--whole-archive",
                "-Xlinker=--no-whole-archive",
            ),
            (
                "gnu-single-xlinker",
                "-Xlinker=-whole-archive",
                "-Xlinker=-no-whole-archive",
            ),
            (
                "abbreviated-xlinker",
                "-Xlinker --whol",
                "-Xlinker --no-whol",
            ),
        )
        for label, before, after in cases:
            with self.subTest(label=label):
                self.write_fixture(self.default_segments)
                # Mutate both recipe and verbose command: the template gate,
                # not an accidental template/log mismatch, must reject it.
                self._set_linker_recipe_controls(before, after)
                with self.assertRaisesRegex(
                    packager.PackagingError, "recipe must not contain whole-archive"
                ):
                    self.package()

        # Prefixes below GNU ld's documented acceptance boundary are ordinary
        # unrelated recipe tokens and must not be overblocked by this guard.
        self.write_fixture(self.default_segments)
        self._set_linker_recipe_controls("-Wl,--w", "-Wl,--no-w")
        self.package()

    def _write_sketch_object_archive(self, archive: Path) -> None:
        archive.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(
            ["ar", "rcs", str(archive), str(self.build_dir / "sketch" / "Demo.ino.cpp.o")],
            check=True,
        )

    def _set_link_map_archive_members(self, archives: list[Path]) -> None:
        link_map = self.build_dir / "Demo.ino.map"
        elf = self.build_dir / "Demo.ino.elf"
        member = "Demo.ino.cpp.o"
        link_map.write_text(
            f"OUTPUT({elf.as_posix()} elf32-littleriscv)\n"
            + "\n".join(f"{archive.as_posix()}({member})" for archive in archives)
            + "\n",
            encoding="utf-8",
        )

    def _package_with_fixture_archive_map(self) -> tuple[Path, Path]:
        # The fixture linker always writes a direct-object map during replay.
        # Preserve that unrelated replay assertion elsewhere while letting this
        # focused test exercise archive-member map parsing before comparison.
        original_canonical_map = packager.canonical_link_map_bytes
        try:
            packager.canonical_link_map_bytes = lambda _raw, _elf: b"fixture-map"
            return self.package()
        finally:
            packager.canonical_link_map_bytes = original_canonical_map

    def test_link_map_archive_members_may_repeat_only_for_one_trusted_archive(self) -> None:
        archive = self.build_dir / "sketch" / "objs.a"
        self._write_sketch_object_archive(archive)
        self._set_link_map_archive_members([archive, archive])
        self._package_with_fixture_archive_map()

        self.write_fixture(self.default_segments)
        first = self.build_dir / "sketch" / "objs.a"
        second = self.build_dir / "other" / "sketch" / "objs.a"
        self._write_sketch_object_archive(first)
        self._write_sketch_object_archive(second)
        self._set_link_map_archive_members([first, second])
        with self.assertRaisesRegex(packager.PackagingError, "must resolve to one archive"):
            self._package_with_fixture_archive_map()

        self.write_fixture(self.default_segments)
        outside = self.root / "outside-map-archive" / "sketch" / "objs.a"
        self._write_sketch_object_archive(outside)
        self._set_link_map_archive_members([outside])
        with self.assertRaisesRegex(packager.PackagingError, "archive escapes"):
            self._package_with_fixture_archive_map()

        self.write_fixture(self.default_segments)
        archive = self.build_dir / "sketch" / "objs.a"
        self._write_sketch_object_archive(archive)
        symlink = self.build_dir / "symlink-map-archive" / "sketch" / "objs.a"
        symlink.parent.mkdir(parents=True)
        symlink.symlink_to(archive)
        self._set_link_map_archive_members([symlink])
        with self.assertRaisesRegex(packager.PackagingError, "not a regular absolute"):
            self._package_with_fixture_archive_map()

    def test_objcopy_requires_final_linked_elf_operand(self) -> None:
        lines = self.properties_path.read_text(encoding="utf-8").splitlines()
        lines = [
            line.replace(' "{build.path}/{build.project_name}.elf"', "")
            if line.startswith("recipe.objcopy.bin.pattern_args=") else line
            for line in lines
        ]
        self.properties_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
        with self.assertRaisesRegex(packager.PackagingError, "one application -o and final linked ELF"):
            self.package()

    def test_compile_command_representation_and_output_are_unambiguous(self) -> None:
        commands_path = self.build_dir / "compile_commands.json"
        commands = json.loads(commands_path.read_text(encoding="utf-8"))
        commands[0]["arguments"] = shlex.split(commands[0]["command"])
        commands_path.write_text(json.dumps(commands) + "\n", encoding="utf-8")
        with self.assertRaisesRegex(packager.PackagingError, "exactly one"):
            self.package()

        self.write_fixture(self.default_segments)
        commands = json.loads(commands_path.read_text(encoding="utf-8"))
        generated = self.build_dir / "sketch" / "Demo.ino.cpp"
        output = self.build_dir / "sketch" / "Demo.ino.cpp.o"
        commands[0]["command"] = (
            f"fake-c++ -c {self.root / 'ActuallyCompiled.cpp'} "
            f"-o {generated.as_posix()}"
        )
        (self.root / "ActuallyCompiled.cpp").write_text(
            "void setup() {}\n", encoding="utf-8"
        )
        commands_path.write_text(json.dumps(commands) + "\n", encoding="utf-8")
        with self.assertRaisesRegex(
            packager.PackagingError, "canonical Arduino source/output layout|expected generated"
        ):
            self.package()

        self.write_fixture(self.default_segments)
        output.unlink()
        with self.assertRaisesRegex(packager.PackagingError, "output is missing"):
            self.package()

    def test_trusted_repository_rejects_hidden_index_flags_and_ignores_git_env(self) -> None:
        source_relative = self.sketch_relative / "Demo.ino"
        self._git("update-index", "--assume-unchanged", source_relative.as_posix())
        (self.sketch_dir / "Demo.ino").write_text(
            "void setup() { int hidden = 1; (void)hidden; }\nvoid loop() {}\n",
            encoding="utf-8",
        )
        with self.assertRaisesRegex(packager.PackagingError, "repository index contains"):
            self.package()

        self._git("update-index", "--no-assume-unchanged", source_relative.as_posix())
        (self.sketch_dir / "Demo.ino").write_text(
            "void setup() {}\nvoid loop() {}\n", encoding="utf-8"
        )
        self._git("update-index", "--skip-worktree", source_relative.as_posix())
        with self.assertRaisesRegex(packager.PackagingError, "repository index contains"):
            self.package()
        self._git("update-index", "--no-skip-worktree", source_relative.as_posix())

        saved = {key: os.environ.get(key) for key in ("GIT_DIR", "GIT_WORK_TREE", "GIT_INDEX_FILE")}
        try:
            os.environ["GIT_DIR"] = (self.root / "attacker.git").as_posix()
            os.environ["GIT_WORK_TREE"] = (self.root / "attacker-tree").as_posix()
            os.environ["GIT_INDEX_FILE"] = (self.root / "attacker.index").as_posix()
            self.package()
        finally:
            for key, value in saved.items():
                if value is None:
                    os.environ.pop(key, None)
                else:
                    os.environ[key] = value

    def test_trusted_repository_rejects_same_size_mtime_spoof_with_trustctime_disabled(self) -> None:
        source = self.sketch_dir / "Demo.ino"
        original_stat = source.stat()
        self._git("config", "core.trustctime", "false")
        # Exactly the same byte count and restored timestamp model the stale-index
        # bypass; the disposable read-tree/index gate must still inspect content.
        source.write_text("void setup(){ }\nvoid loop() {}\n", encoding="utf-8")
        os.utime(source, ns=(original_stat.st_atime_ns, original_stat.st_mtime_ns))
        with self.assertRaisesRegex(packager.PackagingError, "working tree (content|is not clean)"):
            self.package()

    def test_trusted_repository_rejects_local_filters_info_attributes_and_replace_refs(self) -> None:
        self._git("config", "filter.evil.clean", "cat")
        with self.assertRaisesRegex(packager.PackagingError, "clean/process/required filters"):
            self.package()
        self._git("config", "--unset", "filter.evil.clean")

        info_attributes = self.checkout / ".git" / "info" / "attributes"
        info_attributes.parent.mkdir(exist_ok=True)
        info_attributes.write_text("*.ino filter=evil\n", encoding="utf-8")
        with self.assertRaisesRegex(packager.PackagingError, "info/attributes"):
            self.package()
        info_attributes.unlink()

        replacement = self._git(
            "-c", "user.email=a@b", "-c", "user.name=t",
            "commit-tree", "HEAD^{tree}", "-m", "replacement",
        )
        self._git("replace", self.head, replacement)
        with self.assertRaisesRegex(packager.PackagingError, "replacement refs"):
            self.package()

    def test_generated_tu_line_range_and_source_freshness_fail_closed(self) -> None:
        generated = self.build_dir / "sketch" / "Demo.ino.cpp"
        primary_abs = (self.sketch_dir / "Demo.ino").resolve().as_posix()
        generated.write_text(
            '#include "Arduino.h"\n'
            f'#line 99999 "{primary_abs}"\n'
            "void setup() { int forged = 1; (void)forged; }\n"
            "void loop() {}\n",
            encoding="utf-8",
        )
        with self.assertRaisesRegex(packager.PackagingError, "exceeds the primary source"):
            self.package()

        self.write_fixture(self.default_segments)
        (self.sketch_dir / "Demo.ino").write_text(
            "void setup() { int changed = 99; (void)changed; }\nvoid loop() {}\n",
            encoding="utf-8",
        )
        self._git("add", self.sketch_relative.as_posix())
        self._git(
            "-c",
            "user.email=a@b",
            "-c",
            "user.name=t",
            "commit",
            "-q",
            "-m",
            "change source after compile",
        )
        self.head = self._git("rev-parse", "HEAD")
        with self.assertRaisesRegex(
            packager.PackagingError, "differs from the primary sketch source"
        ):
            self.package()

    def test_generated_tu_injected_prototypes_must_match_definition_headers(self) -> None:
        generated = self.build_dir / "sketch" / "Demo.ino.cpp"
        original = generated.read_text(encoding="utf-8")
        for label, replacement in (
            ("injected", "int injected_evil = 42;"),
            ("wrong-signature", "void setup(int attacker);"),
            ("multi-line", "void setup(\n);"),
        ):
            with self.subTest(label=label):
                generated.write_text(original.replace("void setup();", replacement, 1), encoding="utf-8")
                with self.assertRaisesRegex(packager.PackagingError, "injected prototype"):
                    self.package()
        generated.write_text(original, encoding="utf-8")

    def test_replay_binds_object_elf_map_and_application(self) -> None:
        targets = (
            self.build_dir / "sketch" / "Demo.ino.cpp.o",
            self.build_dir / "Demo.ino.elf",
            self.build_dir / "Demo.ino.map",
            self.build_dir / "Demo.ino.bin",
        )
        for target in targets:
            with self.subTest(target=target.name):
                self.write_fixture(self.default_segments)
                target.write_bytes(target.read_bytes() + b"tampered")
                with self.assertRaisesRegex(packager.PackagingError, "replayed|image|link map"):
                    self.package()

    def test_link_map_only_normalizes_one_exact_output_line(self) -> None:
        link_map = self.build_dir / "Demo.ino.map"
        link_map.write_text(
            link_map.read_text(encoding="utf-8")
            + f"OUTPUT({(self.build_dir / 'Demo.ino.elf').as_posix()} elf32-littleriscv)\n",
            encoding="utf-8",
        )
        with self.assertRaisesRegex(packager.PackagingError, "exactly one canonical OUTPUT"):
            self.package()

    def test_binary_privacy_runs_reject_generic_host_paths_without_nul_joining(self) -> None:
        for value in (
            b"\x00/opt/vendor/private-build/source.cpp\x00",
            b"\x00/usr/local/src/private.cpp\x00",
            b"\x00C:\\private\\source.cpp\x00",
            b"\x00\\\\host\\share\\secret.cpp\x00",
        ):
            with self.subTest(value=value):
                with self.assertRaisesRegex(packager.PackagingError, "private host path"):
                    packager.validate_public_artifact_privacy({"bin/test.bin": value})
        # These short adjacent runs must not be concatenated across NUL into a
        # fabricated path that does not exist in the public binary.
        packager.validate_public_artifact_privacy({"bin/test.bin": b"/op\x00t/vendor"})
        observed_random_runs = (
            b"  /l", b" /Ag", b" /cV", b" /qF", b"$ /0", b"/0o0",
            b"/540", b"/LPHL", b"/QG3", b"/XMc", b" /uU", b"@/cN", b"@/qF", b"\x60/cN",
            b"P:\\Uc",
        )
        for random_short_run in observed_random_runs:
            packager.validate_public_artifact_privacy({"bin/test.bin": random_short_run})
        for embedded_or_extended_run in (
            *(value + b"x" for value in observed_random_runs),
            *(b"prefix " + value + b" suffix" for value in observed_random_runs),
        ):
            with self.subTest(embedded_or_extended_run=embedded_or_extended_run):
                with self.assertRaisesRegex(packager.PackagingError, "private host path"):
                    packager.validate_public_artifact_privacy(
                        {"bin/test.bin": embedded_or_extended_run}
                    )
        for private_short_path in (
            b"/app", b"/src", b"/idf",
            b"/uUx", b"/XMcx", b"/540x", b"/0o0x", b"/Agx",
        ):
            with self.subTest(private_short_path=private_short_path):
                with self.assertRaisesRegex(packager.PackagingError, "private host path"):
                    packager.validate_public_artifact_privacy(
                        {"bin/test.bin": b" " + private_short_path}
                    )
        for malformed_short_run in (
            b"@/uU", b":/uU", b" -I/uU", b" -L/uU", b"compiler -I/uU",
            b"/uU/", b"/uU?", b"/uU:x", b"prefix /uU suffix",
        ):
            with self.subTest(malformed_short_run=malformed_short_run):
                with self.assertRaisesRegex(packager.PackagingError, "private host path"):
                    packager.validate_public_artifact_privacy(
                        {"bin/test.bin": malformed_short_run}
                    )
        # A slash following an ordinary label remains a relative-label syntax,
        # rather than an absolute path candidate.
        packager.validate_public_artifact_privacy({"bin/test.bin": b"x/uU"})
        # Stock SDK runtime paths can occur inside a larger diagnostic string;
        # each path candidate is classified independently rather than allowing
        # the whole printable run to mask a later private path.
        packager.validate_public_artifact_privacy(
            {
                "bin/test.bin": (
                    b'esp_vfs_register_fs("/dev/uart", &s_vfs_uart, '
                    b"ESP_VFS_FLAG_STATIC, NULL)"
                )
            }
        )
        with self.assertRaisesRegex(packager.PackagingError, "private host path"):
            packager.validate_public_artifact_privacy(
                {"bin/test.bin": b"stock /IDF/components/hal/foo.c then /opt/private/x.c"}
            )
        for safe_relative_or_url in (
            b"product/examples/arduino/libraries/display.cpp",
            b"arduino-data/packages/esp32/hardware/esp32/core.c",
            b"Compile Date/Time",
            b"BUS_TYPE[bus/unit]",
            b"./managed_components/component/source.c",
            b"/product/examples/arduino/libraries/displays/gt911.cpp",
            b"/./managed_components/espressif__esp_hosted/host/api/src/esp_hosted_api.c",
            b"/./managed_components/espressif__esp_video/src/esp_video.c",
            b"https://example.com/public/reference",
            b"https://example.com/home/public/reference",
        ):
            with self.subTest(safe=safe_relative_or_url):
                packager.validate_public_artifact_privacy(
                    {"bin/test.bin": safe_relative_or_url}
                )

    def test_binary_privacy_allowlist_is_exact_and_context_sensitive(self) -> None:
        for path in (
            b"/IDF/components/hal/foo.c",
            b"//IDF/components/esp_system/start.c",
            b"/builds/idf/crosstool-NG/.build/riscv32-esp-elf/src/newlib/newlib/libc/stdlib/dtoa.c",
            b"//builds/ae_group/esp_ipa/src/esp_ipa_af.c",
            b"//builds/ae_group/esp_ipa/src/esp_ipa_awb.c",
            b"/dev/console", b"/dev/null", b"/dev/secondary", b"/dev/uart", b"/dev/uart/0", b"/dev/esps0",
            b"/dev/video0", b"/dev/video20",
            b"/platform_sd_card.txt", b"/sdcard", b"/spiffs",
        ):
            with self.subTest(path=path):
                packager.validate_public_artifact_privacy({"bin/test.bin": b"diagnostic " + path})
        packager.validate_public_artifact_privacy({"bin/test.bin": b"/dev/%s"})
        with self.assertRaisesRegex(packager.PackagingError, "private host path"):
            packager.validate_public_artifact_privacy(
                {"bin/test.bin": b"diagnostic /dev/%s"}
            )
        for path in (
            b"/IDF/private", b"/builds/idf/crosstool-NG/.build/riscv32-esp-elf/private-user",
            b"/builds/idf/crosstool-NG/.build/riscv32-esp-elf/src/newlib/private-user/source.c",
            b"/dev/ttyUSB0", b"/dev/serial", b"/IDF", b"/opt", b"/IDF/components/../private.c",
            b"/IDF//components/hal/foo.c", b"-I/IDF/components/hal/foo.c",
            b"-L/IDF/components/hal/foo.a", b"@/IDF/components/hal/foo.c",
            b"-isystem/opt/vendor/private/include", b"-iquote/opt/vendor/private/include",
            b"//builds/ae_group/esp_ipa/src/private.c",
            b"//builds/ae_group/esp_ipa/src/esp_ipa_private.c",
            b"X//builds/ae_group/esp_ipa/src/esp_ipa_af.c",
            b"/dev/video1", b"/dev/video21", b"/dev/video0/private", b"/dev/%sx",
            b"/platform_sd_card.txt/private", b"/sdcard/private", b"/spiffs/private",
            b"/platform_sd_card.txt\\private", b"/sdcard\\private", b"/spiffs\\private",
            b"/product/home/ubuntu/private", b"/./managed_components/private/source.c",
            b"/product/examples/arduino/examples/../secret.c",
            b"/./managed_components/espressif__esp_hosted/../private/source.c", b"/sys",
            b"@/./managed_components/espressif__esp_video/src/esp_video.c",
            b":/./managed_components/espressif__esp_video/src/esp_video.c",
            b"-I/./managed_components/espressif__esp_video/src/esp_video.c",
            b"foo:/IDF/components/hal/foo.c",
        ):
            with self.subTest(path=path):
                with self.assertRaisesRegex(packager.PackagingError, "private host path"):
                    packager.validate_public_artifact_privacy({"bin/test.bin": b"diagnostic " + path})

    def test_manifest_numeric_bools_fail_closed(self) -> None:
        archive, _manifest = self.package()
        members = self.read_members(archive)
        manifest = json.loads(members["manifest.json"])
        manifest["flash"]["segments"][0]["offset"] = False
        members["manifest.json"] = self.encode_manifest(manifest)
        self.assert_validator_rejects(
            self.write_mutated_archive(members, "bool-offset.zip"), "numeric fields"
        )

        members = self.read_members(archive)
        identity = json.loads(members["metadata/build_identity.json"])
        identity["schema_version"] = True
        members["metadata/build_identity.json"] = self.encode_manifest(identity)
        self.assert_validator_rejects(
            self.write_mutated_archive(members, "bool-identity-schema.zip"), "build identity.*schema"
        )

        members = self.read_members(archive)
        manifest = json.loads(members["manifest.json"])
        manifest["bsp"]["used_by_build"] = 0
        members["manifest.json"] = self.encode_manifest(manifest)
        self.assert_validator_rejects(
            self.write_mutated_archive(members, "integer-bsp-bool.zip"), "BSP evidence"
        )

    def test_real_additional_recipe_segment_is_preserved_as_other(self) -> None:
        segments = [*self.default_segments, (0x200000, "ota_data.bin", b"X" * 0x1000)]
        self.write_fixture(segments)
        archive, _manifest = self.package()
        manifest = self.validate_archive(
            archive, local_build=self.local_evidence()
        )
        self.assertEqual(manifest["flash"]["segments"][-1]["role"], "other")

    def test_zip_revalidation_recomputes_actual_local_raw_evidence(self) -> None:
        archive, _manifest = self.package()
        members = self.read_members(archive)
        manifest = json.loads(members["manifest.json"])
        manifest["build"]["raw_evidence"]["build_options"] = {
            "basename": "build.options.json",
            "size": 1,
            "sha256": "a" * 64,
        }
        manifest["build"]["build_options_sha256"] = "a" * 64
        members["manifest.json"] = self.encode_manifest(manifest)
        members["metadata/build_identity.json"] = self.encode_manifest(
            {
                "schema_version": 1,
                "build": {
                    key: value
                    for key, value in manifest["build"].items()
                    if key != "identity_metadata_path"
                },
            }
        )
        mutated = self.write_mutated_archive(members, "resigned-raw-evidence.zip")
        with self.assertRaisesRegex(packager.PackagingError, "actual local"):
            self.validate_archive(
                mutated, local_build=self.local_evidence()
            )

    def test_resigned_manifest_label_name_and_path_spoofs_fail_closed(self) -> None:
        archive, _manifest = self.package()
        mutations = {
            "name": lambda manifest: manifest.__setitem__("name", "Other"),
            "path": lambda manifest: manifest.__setitem__(
                "project_path", "examples/arduino/examples/Other"
            ),
            "application-name": lambda manifest: manifest["build"].__setitem__(
                "application_binary_basename", "Other.ino.bin"
            ),
        }
        for label, mutate in mutations.items():
            with self.subTest(label=label):
                members = self.read_members(archive)
                manifest = json.loads(members["manifest.json"])
                mutate(manifest)
                members["manifest.json"] = self.encode_manifest(manifest)
                members["metadata/build_identity.json"] = self.encode_manifest(
                    {
                        "schema_version": 1,
                        "build": {
                            key: value
                            for key, value in manifest["build"].items()
                            if key != "identity_metadata_path"
                        },
                    }
                )
                mutated = self.write_mutated_archive(
                    members, f"resigned-identity-{label}.zip"
                )
                with self.assertRaises(packager.PackagingError):
                    self.validate_archive(mutated)

        members = self.read_members(archive)
        manifest = json.loads(members["manifest.json"])
        manifest["name"] = "Other"
        manifest["project_path"] = "examples/arduino/examples/Other"
        manifest["build"]["sketch_name"] = "Other"
        manifest["build"]["primary_source_basename"] = "Other.ino"
        manifest["build"]["generated_translation_unit_basename"] = "Other.ino.cpp"
        manifest["build"]["application_binary_basename"] = "Other.ino.bin"
        manifest["build"]["raw_evidence"]["primary_source"]["basename"] = "Other.ino"
        manifest["build"]["raw_evidence"]["generated_translation_unit"][
            "basename"
        ] = "Other.ino.cpp"
        members["manifest.json"] = self.encode_manifest(manifest)
        members["metadata/build_identity.json"] = self.encode_manifest(
            {
                "schema_version": 1,
                "build": {
                    key: value
                    for key, value in manifest["build"].items()
                    if key != "identity_metadata_path"
                },
            }
        )
        coordinated = self.write_mutated_archive(
            members, "resigned-coordinated-name-path.zip"
        )
        with self.assertRaisesRegex(
            packager.PackagingError, "actual local|trusted repository|evidence basename"
        ):
            self.validate_archive(
                coordinated, local_build=self.local_evidence()
            )

    def test_archive_rechecks_segment_bytes_hash_and_size(self) -> None:
        archive, _manifest = self.package()
        members = self.read_members(archive)
        members["bin/Demo.ino.bin"] = b"X" + members["bin/Demo.ino.bin"][1:]
        mutated = self.write_mutated_archive(members, "tampered-bytes.zip")
        with self.assertRaisesRegex(packager.PackagingError, "segment differs"):
            self.validate_archive(mutated)

    def test_archive_rechecks_overlap_and_bounds_after_reopen(self) -> None:
        archive, _manifest = self.package()
        for label, old_offset, new_offset in (
            ("overlap", "0x8000", "0x2800"),
            ("out-of-bounds", "0x10000", "0xffff00"),
        ):
            with self.subTest(label=label):
                members = self.read_members(archive)
                raw_name = "metadata/flash_args.source.txt"
                members[raw_name] = members[raw_name].replace(
                    old_offset.encode(), new_offset.encode(), 1
                )
                members["flash_args"] = members["flash_args"].replace(
                    old_offset.encode(), new_offset.encode(), 1
                )
                manifest = json.loads(members["manifest.json"])
                index = 1 if label == "overlap" else 3
                item = manifest["flash"]["segments"][index]
                offset = int(new_offset, 0)
                item["offset"] = offset
                item["offset_hex"] = new_offset
                item["end"] = offset + item["size"]
                item["end_hex"] = f"{item['end']:#x}"
                manifest["flash"]["offset_provenance"]["sha256"] = packager.sha256_bytes(
                    members[raw_name]
                )
                manifest["flash"]["recipe_provenance"]["offsets"][index] = offset
                manifest["flash"]["write_flash_command"] = manifest["flash"][
                    "write_flash_command"
                ].replace(old_offset, new_offset, 1)
                members["manifest.json"] = self.encode_manifest(manifest)
                mutated = self.write_mutated_archive(members, f"tampered-{label}.zip")
                with self.assertRaises(packager.PackagingError):
                    self.validate_archive(mutated)

    def test_each_platform_helper_tamper_fails_closed(self) -> None:
        archive, _manifest = self.package()
        self.assert_validator_subprocess_accepts(archive)
        helpers = {
            "posix": ("flash.sh", "bin/Demo.ino.partitions.bin"),
            "windows": ("flash.cmd", r"bin\Demo.ino.partitions.bin"),
        }
        mutations = {
            "segment-delete": lambda text, segment: text.replace(
                f'0x8000 "{segment}"', "", 1
            ),
            "offset": lambda text, _segment: text.replace("0x8000", "0x9000", 1),
            "filename": lambda text, _segment: text.replace(
                "Demo.ino.partitions.bin", "Wrong.partitions.bin", 1
            ),
            "flash-option": lambda text, _segment: text.replace(
                "--flash-mode dio", "--flash-mode qio", 1
            ),
        }

        for platform, (helper_name, segment_name) in helpers.items():
            with self.subTest(platform=platform, mutation="member-delete"):
                members = self.read_members(archive)
                del members[helper_name]
                mutated = self.write_mutated_archive(
                    members, f"{platform}-helper-member-delete.zip"
                )
                self.assert_validator_rejects(mutated, "missing required members")

            for mutation_name, mutate in mutations.items():
                with self.subTest(platform=platform, mutation=mutation_name):
                    members = self.read_members(archive)
                    original = members[helper_name].decode("utf-8")
                    changed = mutate(original, segment_name)
                    self.assertNotEqual(
                        changed,
                        original,
                        f"{platform} {mutation_name} fixture did not alter the helper",
                    )
                    members[helper_name] = changed.encode("utf-8")
                    mutated = self.write_mutated_archive(
                        members, f"{platform}-helper-{mutation_name}.zip"
                    )
                    self.assert_validator_rejects(
                        mutated, f"flash helper differs.*{helper_name}"
                    )

    def test_manifest_hard_gates_reject_tampering(self) -> None:
        archive, _manifest = self.package()

        def mutation_cases() -> list[tuple[str, object]]:
            return [
                ("product-sha", lambda m: m.__setitem__("product_git_sha", "23c9085")),
                (
                    "payload-total",
                    lambda m: m["flash"].__setitem__(
                        "segmented_payload_total", m["flash"]["segmented_payload_total"] + 1
                    ),
                ),
                (
                    "write-command",
                    lambda m: m["flash"].__setitem__(
                        "write_flash_command", m["flash"]["write_flash_command"] + " --erase-all"
                    ),
                ),
            ]

        for label, mutate in mutation_cases():
            with self.subTest(label=label):
                members = self.read_members(archive)
                manifest = json.loads(members["manifest.json"])
                mutate(manifest)
                members["manifest.json"] = self.encode_manifest(manifest)
                mutated = self.write_mutated_archive(members, f"manifest-{label}.zip")
                with self.assertRaises(packager.PackagingError):
                    self.validate_archive(mutated)

    def test_manifest_artifact_type_and_nested_schemas_are_exact(self) -> None:
        archive, _manifest = self.package()
        cases = (
            (
                "artifact-type",
                lambda manifest: manifest.__setitem__("artifact_type", "whole-flash"),
                "artifact_type",
            ),
            (
                "top-level-extra",
                lambda manifest: manifest.__setitem__("unexpected", True),
                "top-level schema",
            ),
            (
                "framework-extra",
                lambda manifest: manifest["framework"].__setitem__("unexpected", True),
                "framework identity",
            ),
            (
                "build-extra",
                lambda manifest: manifest["build"].__setitem__("unexpected", True),
                "build identity",
            ),
            (
                "toolchain-extra",
                lambda manifest: manifest["toolchain"].__setitem__("unexpected", True),
                "toolchain evidence",
            ),
            (
                "flash-extra",
                lambda manifest: manifest["flash"].__setitem__("unexpected", True),
                "flash record",
            ),
        )
        for label, mutate, expected_error in cases:
            with self.subTest(label=label):
                members = self.read_members(archive)
                manifest = json.loads(members["manifest.json"])
                mutate(manifest)
                members["manifest.json"] = self.encode_manifest(manifest)
                mutated = self.write_mutated_archive(members, f"schema-{label}.zip")
                with self.assertRaisesRegex(packager.PackagingError, expected_error):
                    self.validate_archive(mutated)

    def test_every_bsp_field_is_mandatory_in_archive(self) -> None:
        archive, _manifest = self.package()
        for key in packager.expected_bsp_record():
            with self.subTest(key=key):
                members = self.read_members(archive)
                manifest = json.loads(members["manifest.json"])
                del manifest["bsp"][key]
                members["manifest.json"] = self.encode_manifest(manifest)
                mutated = self.write_mutated_archive(members, f"missing-bsp-{key}.zip")
                with self.assertRaisesRegex(packager.PackagingError, "BSP evidence"):
                    self.validate_archive(mutated)

    def test_every_bsp_cli_value_is_mandatory_and_exact(self) -> None:
        for key in BSP_ARGS:
            with self.subTest(key=key):
                with self.assertRaises(packager.PackagingError):
                    self.package(**{key: ""})
        with self.assertRaisesRegex(packager.PackagingError, "verified Registry"):
            self.package(bsp_version="2.0.0")

    def test_zip_traversal_is_rejected(self) -> None:
        archive_path = self.root / "traversal.zip"
        with zipfile.ZipFile(archive_path, "w") as archive:
            archive.writestr("../outside", b"not safe")
        with self.assertRaisesRegex(packager.PackagingError, "unsafe ZIP member"):
            self.validate_archive(archive_path)

    def test_public_text_privacy_gate_rejects_host_details(self) -> None:
        archive, _manifest = self.package()
        private_values = (
            "/home/ubuntu/.arduino15/tool",
            "/tmp/build/output",
            r"C:\Users\alice\AppData\cache",
            "/workspace/repository/build",
            ".cache/arduino/tool",
            "ubuntu",
        )
        for index, private_value in enumerate(private_values):
            with self.subTest(private_value=private_value):
                members = self.read_members(archive)
                members["requirements.txt"] += private_value.encode() + b"\n"
                members.pop("SHA256SUMS.txt")
                members["SHA256SUMS.txt"] = packager.checksum_document(members)
                path = self.root / f"private-{index}.zip"
                packager.write_zip(path, members)
                with self.assertRaisesRegex(packager.PackagingError, "private (host|username)"):
                    self.validate_archive(path)

    def test_binary_segment_privacy_gate_rejects_printable_host_paths(self) -> None:
        archive, _manifest = self.package()
        members = self.read_members(archive)
        members["bin/Demo.ino.bin"] += b"\x00/home/ubuntu/.arduino15/tool/source.c\x00"
        mutated = self.write_mutated_archive(members, "private-binary-segment.zip")
        with self.assertRaisesRegex(
            packager.PackagingError, "public artifact member exposes a private"
        ):
            self.validate_archive(mutated)

    def test_shell_helper_rejects_wrong_esptool_before_port_access(self) -> None:
        archive, _manifest = self.package()
        members = self.read_members(archive)
        extracted = self.root / "helper-runtime"
        extracted.mkdir()
        for name, data in members.items():
            if name.startswith("bin/") or name == "flash.sh":
                destination = extracted / name
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes(data)
        helper = extracted / "flash.sh"
        helper.chmod(0o755)
        fake_bin = self.root / "fake-bin"
        fake_bin.mkdir()
        marker = self.root / "port-was-accessed"
        fake_python = fake_bin / "python"
        fake_python.write_text(
            "#!/bin/sh\n"
            "if [ \"${1:-}\" = -m ] && [ \"${2:-}\" = esptool ] && "
            "[ \"${3:-}\" = version ]; then\n"
            "  printf 'esptool.py v4.12.0\\n4.12.0\\n'\n"
            "  exit 0\n"
            "fi\n"
            'printf accessed > "$MARKER"\n'
            "exit 88\n",
            encoding="utf-8",
        )
        fake_python.chmod(0o755)
        environment = os.environ.copy()
        environment["PATH"] = f"{fake_bin}:/usr/bin:/bin"
        environment["MARKER"] = marker.as_posix()
        result = subprocess.run(
            ["sh", "flash.sh", "/dev/definitely-not-a-port"],
            cwd=extracted,
            env=environment,
            capture_output=True,
            text=True,
        )
        self.assertEqual(result.returncode, 2)
        self.assertIn("Expected esptool==5.3.1", result.stderr)
        self.assertFalse(marker.exists(), "wrong esptool reached the flash operation")

    def test_forged_line_directive_is_rejected(self) -> None:
        generated_source = self.build_dir / "sketch" / "Demo.ino.cpp"
        other_abs = (self.checkout / "examples/arduino/examples/Other/Other.ino").resolve()
        generated_source.write_text(
            '#include "Arduino.h"\n'
            f'#line 1 "{other_abs.as_posix()}"\n'
            'void setup() {}\n'
            f'#line 2 "{other_abs.as_posix()}"\n'
            'void loop() {}\n',
            encoding="utf-8",
        )
        with self.assertRaisesRegex(packager.PackagingError, "#line directive"):
            self.package()

    def test_zip_comment_and_extra_fields_are_rejected(self) -> None:
        archive, _manifest = self.package()
        members = self.read_members(archive)
        members.pop("SHA256SUMS.txt")
        members["SHA256SUMS.txt"] = packager.checksum_document(members)

        commented = self.root / "commented.zip"
        with zipfile.ZipFile(commented, "w") as target:
            target.comment = b"leaked /home/ubuntu/.arduino15"
            for name in sorted(members):
                data = members[name]
                target.writestr(name, data)
        with self.assertRaisesRegex(packager.PackagingError, "comment"):
            self.validate_archive(commented)

        extra = self.root / "extra.zip"
        leaked = b"leaked /home/ubuntu/.arduino15/tool"
        # A well-formed extra field (header id + size + payload) survives ZIP
        # parsing and must still be rejected by the validator.
        extra_field = struct.pack("<HH", 0xCAFE, len(leaked)) + leaked
        with zipfile.ZipFile(extra, "w") as target:
            for name in sorted(members):
                data = members[name]
                info = zipfile.ZipInfo(name)
                info.extra = extra_field
                target.writestr(info, data)
        with self.assertRaisesRegex(packager.PackagingError, "extra fields"):
            self.validate_archive(extra)

    def test_zip_local_record_chain_and_local_only_extra_fail_closed(self) -> None:
        archive, _manifest = self.package()

        def insert_and_rebase(raw: bytearray, at: int, payload: bytes) -> bytearray:
            old_eocd = raw.rfind(b"PK\x05\x06")
            old_central = struct.unpack_from("<L", raw, old_eocd + 16)[0]
            total = struct.unpack_from("<H", raw, old_eocd + 10)[0]
            raw[at:at] = payload
            delta = len(payload)
            eocd = old_eocd + delta
            central = old_central + delta
            struct.pack_into("<L", raw, eocd + 16, central)
            position = central
            for _ in range(total):
                self.assertEqual(raw[position : position + 4], b"PK\x01\x02")
                local_offset = struct.unpack_from("<L", raw, position + 42)[0]
                if local_offset >= at:
                    struct.pack_into("<L", raw, position + 42, local_offset + delta)
                name_length, extra_length, comment_length = struct.unpack_from(
                    "<HHH", raw, position + 28
                )
                position += 46 + name_length + extra_length + comment_length
            return raw

        local_extra = self.root / "local-header-only-extra.zip"
        raw = bytearray(archive.read_bytes())
        name_length = struct.unpack_from("<H", raw, 26)[0]
        extra_length = struct.unpack_from("<H", raw, 28)[0]
        leak = b"/home/private/.arduino15/cache"
        field = struct.pack("<HH", 0xCAFE, len(leak)) + leak
        insertion = 30 + name_length + extra_length
        struct.pack_into("<H", raw, 28, extra_length + len(field))
        local_extra.write_bytes(insert_and_rebase(raw, insertion, field))
        with zipfile.ZipFile(local_extra) as parsed:
            self.assertIsNone(parsed.testzip())
            self.assertTrue(all(not info.extra for info in parsed.infolist()))
        with self.assertRaisesRegex(packager.PackagingError, "local-file extra fields"):
            self.validate_archive(local_extra)

        orphan = self.root / "orphan-local-prefix.zip"
        buffer = io.BytesIO()
        with zipfile.ZipFile(buffer, "w", compression=zipfile.ZIP_STORED) as temporary:
            temporary.writestr(
                "orphan-private.txt", b"/home/private/.arduino15/cache"
            )
        orphan_raw = bytearray(buffer.getvalue())
        orphan_eocd = orphan_raw.rfind(b"PK\x05\x06")
        orphan_central = struct.unpack_from("<L", orphan_raw, orphan_eocd + 16)[0]
        orphan.write_bytes(
            insert_and_rebase(
                bytearray(archive.read_bytes()), 0, bytes(orphan_raw[:orphan_central])
            )
        )
        with zipfile.ZipFile(orphan) as parsed:
            self.assertIsNone(parsed.testzip())
            self.assertNotIn("orphan-private.txt", parsed.namelist())
        with self.assertRaisesRegex(packager.PackagingError, "contiguous chain"):
            self.validate_archive(orphan)

    def test_zip_member_comment_mode_and_trailing_bytes_fail_closed(self) -> None:
        archive, _manifest = self.package()
        members = self.read_members(archive)

        def write_with_metadata(
            path: Path,
            *,
            member_comment: bytes = b"",
            manifest_mode: int = 0o644,
            manifest_low_attributes: int = 0,
        ) -> None:
            with zipfile.ZipFile(
                path, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9
            ) as target:
                for name in sorted(members):
                    info = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
                    info.create_system = 3
                    mode = (
                        manifest_mode
                        if name == "manifest.json"
                        else packager.zip_member_mode(name)
                    )
                    info.external_attr = (stat.S_IFREG | mode) << 16
                    if name == "manifest.json":
                        info.external_attr |= manifest_low_attributes
                    info.compress_type = zipfile.ZIP_DEFLATED
                    if name == "manifest.json":
                        info.comment = member_comment
                    target.writestr(info, members[name])

        commented_member = self.root / "member-comment.zip"
        write_with_metadata(
            commented_member, member_comment=b"private /home/ubuntu/.arduino15"
        )
        with self.assertRaisesRegex(packager.PackagingError, "member comments"):
            self.validate_archive(commented_member)

        wrong_mode = self.root / "wrong-mode.zip"
        write_with_metadata(wrong_mode, manifest_mode=0o777)
        with self.assertRaisesRegex(packager.PackagingError, "mode is non-canonical"):
            self.validate_archive(wrong_mode)

        wrong_low_attributes = self.root / "wrong-low-attributes.zip"
        write_with_metadata(wrong_low_attributes, manifest_low_attributes=3)
        with self.assertRaisesRegex(packager.PackagingError, "mode is non-canonical"):
            self.validate_archive(wrong_low_attributes)

        trailing = self.root / "trailing-private-data.zip"
        packager.write_zip(trailing, members)
        with trailing.open("ab") as handle:
            handle.write(b"/home/ubuntu/.arduino15/private")
        with self.assertRaisesRegex(packager.PackagingError, "trailing bytes"):
            self.validate_archive(trailing)

    def test_zip_central_and_local_versions_fail_closed(self) -> None:
        archive, _manifest = self.package()
        raw = bytearray(archive.read_bytes())
        eocd = raw.rfind(b"PK\x05\x06")
        central = struct.unpack_from("<L", raw, eocd + 16)[0]
        self.assertEqual(raw[central : central + 4], b"PK\x01\x02")
        central_version = self.root / "central-version.zip"
        changed = bytearray(raw)
        changed[central + 4] = 63  # version-made-by low byte
        central_version.write_bytes(changed)
        with self.assertRaisesRegex(packager.PackagingError, "encoding|metadata|mode"):
            self.validate_archive(central_version)
        local_version = self.root / "local-version.zip"
        changed = bytearray(raw)
        changed[4] = 63  # local version-needed low byte
        local_version.write_bytes(changed)
        with self.assertRaisesRegex(packager.PackagingError, "metadata"):
            self.validate_archive(local_version)

    def test_zip_order_reserved_and_deflate_tail_fail_closed(self) -> None:
        archive, _manifest = self.package()
        members = self.read_members(archive)
        reverse = self.root / "reverse-order.zip"
        with zipfile.ZipFile(reverse, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as target:
            for name in sorted(members, reverse=True):
                info = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
                info.create_system = 3
                info.create_version = 20
                info.extract_version = 20
                info.external_attr = (stat.S_IFREG | packager.zip_member_mode(name)) << 16
                info.compress_type = zipfile.ZIP_DEFLATED
                target.writestr(info, members[name])
        with self.assertRaisesRegex(packager.PackagingError, "central member order"):
            self.validate_archive(reverse)

        raw = bytearray(archive.read_bytes())
        eocd = raw.rfind(b"PK\x05\x06")
        central = struct.unpack_from("<L", raw, eocd + 16)[0]
        self.assertEqual(raw[central : central + 4], b"PK\x01\x02")
        reserved = self.root / "central-reserved.zip"
        changed = bytearray(raw)
        struct.pack_into("<H", changed, central + 34, 1)
        reserved.write_bytes(changed)
        with self.assertRaisesRegex(packager.PackagingError, "mode is non-canonical|complete non-empty disk"):
            self.validate_archive(reserved)

        # Append bytes inside the first compressed member and consistently
        # rebase ZIP offsets; raw DEFLATE validation must reject the tail.
        changed = bytearray(raw)
        name_length = struct.unpack_from("<H", changed, 26)[0]
        extra_length = struct.unpack_from("<H", changed, 28)[0]
        compressed_size = struct.unpack_from("<L", changed, 18)[0]
        data_end = 30 + name_length + extra_length + compressed_size
        tail = b"JUNK"
        changed[data_end:data_end] = tail
        new_eocd = eocd + len(tail)
        new_central = central + len(tail)
        struct.pack_into("<L", changed, 18, compressed_size + len(tail))
        struct.pack_into("<L", changed, new_central + 20, compressed_size + len(tail))
        total = struct.unpack_from("<H", changed, new_eocd + 10)[0]
        position = new_central
        for _index in range(total):
            self.assertEqual(changed[position : position + 4], b"PK\x01\x02")
            local_offset = struct.unpack_from("<L", changed, position + 42)[0]
            if local_offset >= data_end:
                struct.pack_into("<L", changed, position + 42, local_offset + len(tail))
            name_size, extra_size, comment_size = struct.unpack_from("<HHH", changed, position + 28)
            position += 46 + name_size + extra_size + comment_size
        struct.pack_into("<L", changed, new_eocd + 16, new_central)
        deflate_tail = self.root / "deflate-tail.zip"
        deflate_tail.write_bytes(changed)
        with self.assertRaisesRegex(packager.PackagingError, "DEFLATE payload"):
            self.validate_archive(deflate_tail)

    def test_build_segment_symlink_escaping_build_dir_is_rejected(self) -> None:
        outside = self.root / "outside.bin"
        outside.write_bytes(b"E" * 0x1000)
        escaping = self.build_dir / "Demo.ino.partitions.bin"
        escaping.unlink()
        escaping.symlink_to(outside)
        with self.assertRaisesRegex(packager.PackagingError, "symlink"):
            self.package()

    def test_product_sha_must_match_packager_own_head(self) -> None:
        with self.assertRaisesRegex(
            packager.PackagingError, "does not match the packager's own repository HEAD"
        ):
            self.package(git_sha="f" * 40)

    def test_ab_checkout_same_sketch_name_different_source_fails_closed(self) -> None:
        archive_a, _manifest_a = self.package()

        checkout_b = self.root / "checkout-b"
        scripts_b = checkout_b / ".github" / "scripts"
        scripts_b.mkdir(parents=True)
        shutil.copy(
            SCRIPTS_ROOT / "package_arduino_firmware.py",
            scripts_b / "package_arduino_firmware.py",
        )
        sketch_b = checkout_b / "examples" / "arduino" / "examples" / "Demo"
        sketch_b.mkdir(parents=True)
        (sketch_b / "Demo.ino").write_text(
            "void setup() { int changed = 99; (void)changed; }\nvoid loop() {}\n",
            encoding="utf-8",
        )
        subprocess.run(["git", "init", "-q"], cwd=checkout_b, check=True)
        subprocess.run(
            ["git", "-c", "user.email=a@b", "-c", "user.name=t", "add", "-A"],
            cwd=checkout_b,
            check=True,
        )
        subprocess.run(
            [
                "git",
                "-c",
                "user.email=a@b",
                "-c",
                "user.name=t",
                "commit",
                "-q",
                "-m",
                "alternate source",
            ],
            cwd=checkout_b,
            check=True,
        )
        head_b = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            cwd=checkout_b,
            capture_output=True,
            text=True,
            check=True,
        ).stdout.strip()
        self.assertNotEqual(head_b, self.head)

        # The fixture tools bind replays to a specific trusted checkout.  This
        # independent checkout deliberately uses the same tools, so retarget
        # their exact-root guard; an unrelated caller directory remains
        # rejected by the cwd regression above.
        original_guard = f"expected_cwd={shlex.quote(self.checkout.resolve().as_posix())}"
        alternate_guard = f"expected_cwd={shlex.quote(checkout_b.resolve().as_posix())}"
        for tool in (
            self.root / "toolchain" / "bin" / "riscv32-esp-elf-g++",
            self.tool_dir / "esptool",
        ):
            tool_text = tool.read_text(encoding="utf-8")
            self.assertIn(original_guard, tool_text)
            tool.write_text(tool_text.replace(original_guard, alternate_guard), encoding="utf-8")

        spec_b = importlib.util.spec_from_file_location(
            "package_arduino_firmware_under_test_b",
            scripts_b / "package_arduino_firmware.py",
        )
        assert spec_b is not None and spec_b.loader is not None
        module_b = importlib.util.module_from_spec(spec_b)
        sys.modules[spec_b.name] = module_b
        spec_b.loader.exec_module(module_b)

        build_b = self.root / "build-b"
        shutil.copytree(self.build_dir, build_b)
        options_b = json.loads((build_b / "build.options.json").read_text(encoding="utf-8"))
        options_b["sketchLocation"] = sketch_b.resolve().as_posix()
        (build_b / "build.options.json").write_text(
            json.dumps(options_b) + "\n", encoding="utf-8"
        )
        generated_b = build_b / "sketch" / "Demo.ino.cpp"
        primary_b = (sketch_b / "Demo.ino").resolve().as_posix()
        generated_b.write_text(
            "#include <Arduino.h>\n"
            f'#line 1 "{primary_b}"\n'
            f'#line 1 "{primary_b}"\n'
            "void setup();\n"
            f'#line 2 "{primary_b}"\n'
            "void loop();\n"
            f'#line 1 "{primary_b}"\n'
            "void setup() { int changed = 99; (void)changed; }\n"
            "void loop() {}\n",
            encoding="utf-8",
        )
        object_b = build_b / "sketch" / "Demo.ino.cpp.o"
        object_b.write_bytes(b"compiled-object")
        elf_b = build_b / "Demo.ino.elf"
        elf_b.write_bytes(b"linked-elf")
        map_b = build_b / "Demo.ino.map"
        map_b.write_text(
            f"OUTPUT({elf_b.as_posix()} elf32-littleriscv)\nLOAD {object_b.as_posix()}\n",
            encoding="utf-8",
        )
        (build_b / "compile_commands.json").write_text(
            json.dumps(
                [
                    {
                        "directory": checkout_b.resolve().as_posix(),
                        "file": generated_b.resolve().as_posix(),
                        "command": (
                            f"{(self.root / 'toolchain/bin/riscv32-esp-elf-g++').as_posix()} -c {generated_b.resolve().as_posix()} "
                            f"-o {object_b.resolve().as_posix()}"
                        ),
                    }
                ]
            )
            + "\n",
            encoding="utf-8",
        )
        (build_b / "Demo.ino.bin").write_bytes(b"A" * 0x2000)
        compile_log_b = self.root / "compile-b.verbose.log"
        compile_log_b.write_text(
            "\n".join(
                (
                    f"{(self.root / 'toolchain/bin/riscv32-esp-elf-g++').as_posix()} -c {generated_b.as_posix()} -o {object_b.as_posix()}",
                    f"{(self.root / 'toolchain/bin/riscv32-esp-elf-g++').as_posix()} -Wl,--Map={map_b.as_posix()} -Wl,--start-group {object_b.as_posix()} {(build_b / 'core/core.a').as_posix()} -Wl,--end-group -o {elf_b.as_posix()}",
                    f"{(self.tool_dir / 'esptool').as_posix()} --chip esp32p4 elf2image -o {(build_b / 'Demo.ino.bin').as_posix()} {elf_b.as_posix()}",
                )
            ) + "\n",
            encoding="utf-8",
        )

        args_b = self.arguments(
            build_dir=build_b,
            output_dir=self.root / "output-b",
            git_sha=head_b,
            compile_log=compile_log_b,
        )
        archive_b, manifest_b = module_b.package(args_b)
        evidence_b = module_b.LocalBuildEvidence(
            build_dir=build_b,
            board_properties=self.properties_path,
            compile_log=compile_log_b,
        )
        validated_b = module_b.validate_archive(
            archive_b,
            local_build=evidence_b,
            expected_manifest=manifest_b.read_bytes(),
        )
        self.assertEqual(validated_b["product_git_sha"], head_b)

        with self.assertRaisesRegex(
            module_b.PackagingError, "validator's trusted repository HEAD"
        ):
            module_b.validate_archive(
                archive_a,
                local_build=module_b.LocalBuildEvidence(
                    build_dir=self.build_dir,
                    board_properties=self.properties_path,
                    compile_log=self.compile_log_path,
                ),
            )

        with self.assertRaisesRegex(
            packager.PackagingError, "validator's trusted repository HEAD"
        ):
            packager.validate_archive(
                archive_b,
                local_build=packager.LocalBuildEvidence(
                    build_dir=build_b,
                    board_properties=self.properties_path,
                    compile_log=compile_log_b,
                ),
            )

        with self.assertRaisesRegex(
            packager.PackagingError, "actual local build/source file"
        ):
            packager.validate_archive(
                archive_a,
                local_build=packager.LocalBuildEvidence(
                    build_dir=build_b,
                    board_properties=self.properties_path,
                    compile_log=compile_log_b,
                ),
            )

        with self.assertRaisesRegex(
            module_b.PackagingError, "does not match the packager's own repository HEAD"
        ):
            module_b.package(
                self.arguments(
                    build_dir=build_b,
                    output_dir=self.root / "output-b-wrong-sha",
                    git_sha=self.head,
                    compile_log=compile_log_b,
                )
            )
        with self.assertRaisesRegex(
            packager.PackagingError, "does not match the packager's own repository HEAD"
        ):
            self.package(git_sha=head_b)


if __name__ == "__main__":
    unittest.main()
