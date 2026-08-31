#!/usr/bin/env python3

import re
import shutil
import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[3]
SKETCH_ROOT = REPO_ROOT / "examples" / "arduino" / "examples"
COMMON_ROOT = REPO_ROOT / "examples" / "arduino" / "common"
HELPER = COMMON_ROOT / "serial_log.h"
SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".h", ".hh", ".hpp", ".ino"}
SERIAL_OBJECT = r"(?:Serial[0-9]*|USBSerial|HWCDCSerial)"

EXPECTED_SKETCHES = {
    "AsciiTable",
    "Audio_Playback",
    "Camera_ISP_Tuning",
    "Camera_Preview",
    "Drawing_board",
    "GFX_ESPWiFiAnalyzer",
    "HelloWorld",
    "LVGLV9_Arduino",
    "SD_Card",
}

READINESS_WAIT_PATTERNS = (
    re.compile(rf"\bwhile\s*\([^)]*\b{SERIAL_OBJECT}\b[^)]*\)"),
    re.compile(rf"\bfor\s*\([^;]*;[^;]*\b{SERIAL_OBJECT}\b[^;]*;"),
)


class ArduinoSerialReadinessTests(unittest.TestCase):
    def setUp(self):
        self.sketches = sorted(SKETCH_ROOT.glob("*/*.ino"))
        self.first_party_sources = sorted(
            path
            for root in (SKETCH_ROOT, COMMON_ROOT)
            for path in root.rglob("*")
            if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES
        )

    def test_all_nine_first_party_sketches_use_shared_non_waiting_initializer(self):
        self.assertEqual({path.parent.name for path in self.sketches}, EXPECTED_SKETCHES)
        self.assertTrue(HELPER.is_file(), "shared serial log helper is missing")

        for sketch in self.sketches:
            source = sketch.read_text(encoding="utf-8")
            with self.subTest(sketch=sketch.relative_to(REPO_ROOT)):
                self.assertIn('#include "../../common/serial_log.h"', source)
                self.assertEqual(source.count("waveshare::logging::beginSerialLog();"), 1)
                self.assertNotRegex(source, r"\b(?:Serial|USBSerial)\s*\.\s*begin\s*\(")

    def test_no_serial_readiness_loop_even_in_copyable_comments(self):
        self.assertTrue(
            self.first_party_sources,
            "no first-party Arduino serial sources were found",
        )
        self.assertNotIn(
            REPO_ROOT / "examples" / "arduino" / "libraries",
            {parent for path in self.first_party_sources for parent in path.parents},
            "bundled library examples must remain outside the first-party readiness scan",
        )

        for path in self.first_party_sources:
            source = path.read_text(encoding="utf-8")
            for pattern in READINESS_WAIT_PATTERNS:
                with self.subTest(path=path.relative_to(REPO_ROOT), pattern=pattern.pattern):
                    self.assertIsNone(pattern.search(source), "Serial readiness must never gate startup")

    def test_readiness_patterns_cover_uart_and_both_cdc_serial_objects(self):
        for serial_object in ("Serial", "Serial0", "Serial2", "USBSerial", "HWCDCSerial"):
            with self.subTest(serial_object=serial_object):
                source = f"while (!{serial_object}) {{ /* blocks startup */ }}"
                self.assertTrue(
                    any(pattern.search(source) for pattern in READINESS_WAIT_PATTERNS),
                    f"readiness detector missed {serial_object}",
                )

    def test_disconnected_native_usb_cdc_logging_is_an_immediate_short_write(self):
        self._compile_and_run_harness(serial_mode="usb_cdc")

    def test_disconnected_hardware_cdc_logging_is_an_immediate_buffered_write(self):
        self._compile_and_run_harness(serial_mode="hardware_cdc")

    def test_uart_initializer_does_not_require_cdc_api(self):
        self._compile_and_run_harness(serial_mode="uart")

    def _compile_and_run_harness(self, *, serial_mode: str):
        compiler = shutil.which("c++")
        self.assertIsNotNone(compiler, "a host C++ compiler is required for this fail-closed test")

        cdc_on_boot = serial_mode != "uart"
        if cdc_on_boot:
            cdc_method = """
              void setTxTimeoutMs(uint32_t timeout) {
                tx_timeout_ms = timeout;
              }
            """
        else:
            cdc_method = ""

        if serial_mode == "usb_cdc":
            println_method = """
              size_t println(const char *message) {
                if (!connected) {
                  fake_millis += tx_timeout_ms;
                  return 0;
                }
                return strlen(message) + 2;
              }
            """
        else:
            println_method = """
              size_t println(const char *message) {
                return strlen(message) + 2;
              }
            """

        serial_model = textwrap.dedent(
            f"""
            #pragma once
            #include <stddef.h>
            #include <stdint.h>
            #include <string.h>

            extern uint32_t fake_millis;

            class FakeSerial {{
             public:
              bool connected = false;
              unsigned long baud = 0;
              uint32_t tx_timeout_ms = 250;

              void begin(unsigned long value) {{
                baud = value;
              }}

              {cdc_method}

              {println_method}
            }};

            extern FakeSerial Serial;
            """
        )

        expected_timeout = 0 if cdc_on_boot else 250
        expected_written = 0 if serial_mode == "usb_cdc" else 6
        usb_mode = 1 if serial_mode == "hardware_cdc" else 0
        harness = textwrap.dedent(
            f"""
            #include "Arduino.h"

            uint32_t fake_millis = 0;
            FakeSerial Serial;

            #include "examples/arduino/common/serial_log.h"

            int main() {{
              waveshare::logging::beginSerialLog();
              if (Serial.baud != 115200 || Serial.tx_timeout_ms != {expected_timeout}) {{
                return 1;
              }}

              const size_t written = Serial.println("boot");
              if (written != {expected_written} || fake_millis != 0) {{
                return 2;
              }}
              return 0;
            }}
            """
        )

        with tempfile.TemporaryDirectory() as temp_dir:
            temp = Path(temp_dir)
            (temp / "Arduino.h").write_text(serial_model, encoding="utf-8")
            (temp / "harness.cpp").write_text(harness, encoding="utf-8")
            executable = temp / "serial-readiness-test"

            compile_result = subprocess.run(
                [
                    compiler,
                    "-std=c++17",
                    "-Wall",
                    "-Wextra",
                    "-Werror",
                    f"-DARDUINO_USB_CDC_ON_BOOT={int(cdc_on_boot)}",
                    f"-DARDUINO_USB_MODE={usb_mode}",
                    "-I",
                    str(temp),
                    "-I",
                    str(REPO_ROOT),
                    str(temp / "harness.cpp"),
                    "-o",
                    str(executable),
                ],
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertEqual(
                compile_result.returncode,
                0,
                f"host harness compilation failed:\n{compile_result.stdout}{compile_result.stderr}",
            )

            run_result = subprocess.run(
                [str(executable)],
                check=False,
                capture_output=True,
                text=True,
                timeout=5,
            )
            self.assertEqual(
                run_result.returncode,
                0,
                f"host harness failed:\n{run_result.stdout}{run_result.stderr}",
            )


if __name__ == "__main__":
    unittest.main()
