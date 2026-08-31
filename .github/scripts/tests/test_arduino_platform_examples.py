#!/usr/bin/env python3

import re
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[3]
EXAMPLES = REPO_ROOT / "examples" / "arduino" / "examples"
GFX_SOURCE = EXAMPLES.parent / "libraries" / "GFX_Library_for_Arduino" / "src"
DSI_PANEL_SOURCE = GFX_SOURCE / "databus" / "Arduino_ESP32DSIPanel.cpp"
DSI_DISPLAY_SOURCE = GFX_SOURCE / "display" / "Arduino_DSI_Display.cpp"
DSI_SKETCHES = ("AsciiTable", "Camera_ISP_Tuning", "Camera_Preview", "Drawing_board", "GFX_ESPWiFiAnalyzer", "HelloWorld", "LVGLV9_Arduino")
PLATFORM_SKETCHES = {
    "Camera_Preview": EXAMPLES / "Camera_Preview" / "Camera_Preview.ino",
    "Camera_ISP_Tuning": EXAMPLES / "Camera_ISP_Tuning" / "Camera_ISP_Tuning.ino",
    "SD_Card": EXAMPLES / "SD_Card" / "SD_Card.ino",
    "Audio_Playback": EXAMPLES / "Audio_Playback" / "Audio_Playback.ino",
}


def _region(source, first, last):
    if source.count(first) != 1 or source.count(last) != 1:
        raise AssertionError("unexpected source boundary")
    start = source.index(first)
    return source[start : source.index(last, start + len(first))]


def _active_at(source, position):
    return source.rfind("/*", 0, position) <= source.rfind("*/", 0, position) and source.rfind("//", source.rfind("\n", 0, position), position) < 0


def validate_dsi_panel_clock_contract(source):
    body = _region(source, "bool Arduino_ESP32DSIPanel::begin(", "uint16_t *Arduino_ESP32DSIPanel::getFrameBuffer()")
    config = r"esp_lcd_dsi_bus_config_t\s+bus_config\s*=\s*\{"
    typed_zero = ".phy_clk_src = static_cast<mipi_dsi_phy_pllref_clock_source_t>(0),"
    bus_call = "esp_lcd_new_dsi_bus(&bus_config, &mipi_dsi_bus)"
    configs = list(re.finditer(config, body))
    forbidden = r"(?:#\s*if\b|R\"|MIPI_DSI_PHY_CLK_SRC_DEFAULT|MIPI_DSI_PHY_PLLREF_CLK_SRC_(?:DEFAULT(?:_LEGACY)?|XTAL|PLL_F20M|CPLL)|SOC_MOD_CLK_(?:XTAL|PLL_F20M|CPLL))"
    if (
        len(re.findall(r"\besp_lcd_dsi_bus_config_t\b", source)) != 1
        or len(re.findall(r"\besp_lcd_dsi_bus_config_t\b", body)) != 1
        or len(re.findall(config, source)) != 1
        or len(configs) != 1
        or source.count("phy_clk_src") != 1
        or body.count(typed_zero) != 1
        or source.count(bus_call) != 1
        or body.count(bus_call) != 1
        or len(re.findall(r"esp_lcd_new_dsi_bus\s*\(", source)) != 1
        or len(re.findall(r"esp_lcd_new_dsi_bus\s*\(", body)) != 1
        or not _active_at(body, configs[0].start())
        or not _active_at(body, body.index(typed_zero))
        or not _active_at(body, body.index(bus_call))
        or re.search(forbidden, body)
        or re.search(r"bus_config\s*\.\s*phy_clk_src\s*=", body)
        or re.search(r"static_cast<mipi_dsi_phy_pllref_clock_source_t>\((?!0\))", body)
    ):
        raise AssertionError("DSI bus clock contract changed")
    if not body.index("esp_lcd_dsi_bus_config_t") < body.index(typed_zero) < body.index(bus_call):
        raise AssertionError("DSI bus clock contract order changed")


def validate_dsi_display_contract(source):
    constructor = _region(source, "Arduino_DSI_Display::Arduino_DSI_Display(", "bool Arduino_DSI_Display::begin(")
    begin = _region(source, "bool Arduino_DSI_Display::begin(", "void Arduino_DSI_Display::writePixelPreclipped(")
    initializer = "_dsipanel(dsipanel)"
    start = "_dsipanel->begin("
    initializers = list(re.finditer(re.escape(initializer), constructor))
    starts = list(re.finditer(r"_dsipanel\s*->\s*begin\s*\(", begin))
    if len(initializers) != 1 or len(starts) != 1 or not _active_at(constructor, initializers[0].start()) or not _active_at(begin, starts[0].start()) or re.search(r"\b_dsipanel\s*=", constructor + begin) or re.search(r'#\s*if\b|R"|\[[^]]*\]\s*(?:\([^)]*\)\s*)?\{', constructor + begin):
        raise AssertionError("DSI display no longer starts its retained panel directly")


def validate_dsi_sketch_path(source):
    panel = r"Arduino_ESP32DSIPanel\s*\*\s*dsipanel\s*=\s*new\s+Arduino_ESP32DSIPanel\s*\("
    display = r"Arduino_DSI_Display\s*\*\s*gfx\s*=\s*new\s+Arduino_DSI_Display\s*\(\s*display_cfg\.width\s*,\s*display_cfg\.height\s*,\s*dsipanel\s*,"
    setup = _region(source, "void setup", "void loop(")
    panels, displays = list(re.finditer(panel, source)), list(re.finditer(display, source))
    begins = list(re.finditer(r"if\s*\(\s*!gfx\s*->\s*begin\s*\(\s*\)\s*\)", setup))
    if len(panels) != 1 or len(displays) != 1 or len(begins) != 1 or not _active_at(source, panels[0].start()) or not _active_at(source, displays[0].start()) or not _active_at(setup, begins[0].start()) or re.search(r"\bgfx\s*=", setup[:begins[0].start()]) or re.search(r'#\s*if\b|R"', setup) or re.search(r'\[[^]]*\]\s*(?:\([^)]*\)\s*)?\{', setup):
        raise AssertionError("DSI sketch construction path changed")


class ArduinoPlatformExamplesTests(unittest.TestCase):
    def setUp(self):
        self.sources = {
            name: path.read_text(encoding="utf-8") for name, path in PLATFORM_SKETCHES.items()
        }

    def test_expected_product_sketches_exist(self):
        self.assertTrue(all(path.is_file() for path in PLATFORM_SKETCHES.values()))

    def test_dsi_phy_reference_clock_follows_the_configured_p4_revision(self):
        actual_dsi_sketches = set()
        for sketch in EXAMPLES.glob("*/*.ino"):
            source = sketch.read_text(encoding="utf-8")
            callers = re.finditer(r"\bnew\s+Arduino_ESP32DSIPanel\s*\(", source)
            if any(_active_at(source, caller.start()) for caller in callers):
                actual_dsi_sketches.add(sketch.parent.name)
        self.assertEqual(actual_dsi_sketches, set(DSI_SKETCHES))
        for sketch_name in DSI_SKETCHES:
            with self.subTest(sketch=sketch_name):
                validate_dsi_sketch_path((EXAMPLES / sketch_name / f"{sketch_name}.ino").read_text(encoding="utf-8"))
        validate_dsi_display_contract(DSI_DISPLAY_SOURCE.read_text(encoding="utf-8"))
        validate_dsi_panel_clock_contract(DSI_PANEL_SOURCE.read_text(encoding="utf-8"))

    def test_dsi_clock_contract_rejects_dead_or_fixed_clock_configurations(self):
        source = DSI_PANEL_SOURCE.read_text(encoding="utf-8")
        expected = ".phy_clk_src = static_cast<mipi_dsi_phy_pllref_clock_source_t>(0),"
        duplicate = "esp_lcd_dsi_bus_config_t unused_config = {};\n  ESP_ERROR_CHECK(esp_lcd_new_dsi_bus(&unused_config, &unused_bus));\n  "
        cases = (
            source.replace(expected, ".phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,", 1),
            source.replace(expected, ".phy_clk_src = static_cast<mipi_dsi_phy_pllref_clock_source_t>(1),", 1),
            source.replace(expected, ".phy_clk_src = SOC_MOD_CLK_CPLL,", 1),
            source.replace(expected, f"/* {expected} */", 1),
            source.replace(expected, f"# if (0)\n{expected}\n# endif", 1),
            source.replace(expected, f'R"({expected})"', 1),
            source.replace("esp_lcd_dsi_bus_handle_t mipi_dsi_bus", duplicate + "esp_lcd_dsi_bus_handle_t mipi_dsi_bus", 1),
            source.replace("esp_lcd_new_dsi_bus", "esp_lcd_new_dsi_bus_dead", 1),
        )
        for candidate in cases:
            with self.assertRaises(AssertionError): validate_dsi_panel_clock_contract(candidate)
        display = DSI_DISPLAY_SOURCE.read_text(encoding="utf-8")
        for candidate in (display.replace("\n  _dsipanel->begin(", "\n  // _dsipanel->begin(", 1), display.replace("_fb_width =", "_dsipanel = nullptr;\n  _fb_width =", 1), display.replace("\n  _dsipanel->begin(", "\n  _dsipanel = nullptr;\n  _dsipanel->begin(", 1)):
            with self.assertRaises(AssertionError): validate_dsi_display_contract(candidate)
        sketch = (EXAMPLES / "HelloWorld" / "HelloWorld.ino").read_text(encoding="utf-8")
        spoof = sketch.replace("Arduino_DSI_Display *gfx", "/* Arduino_DSI_Display *gfx = new Arduino_DSI_Display(display_cfg.width, display_cfg.height, dsipanel, */\nArduino_DSI_Display *gfx", 1).replace("  dsipanel,\n  0,", "  nullptr,\n  0,", 1)
        with self.assertRaises(AssertionError): validate_dsi_sketch_path(spoof)
        with self.assertRaises(AssertionError): validate_dsi_sketch_path(sketch.replace("  if (!gfx->begin())", "  gfx = nullptr;\n  if (!gfx->begin())", 1))

    def test_each_sketch_uses_the_shared_nonblocking_serial_initializer_once(self):
        for name, source in self.sources.items():
            with self.subTest(sketch=name):
                self.assertIn("#include <Waveshare_Arduino_Logging.h>", source)
                self.assertEqual(source.count("waveshare::logging::beginSerialLog();"), 1)
                self.assertNotRegex(source, r"\b(?:Serial|USBSerial)\s*\.\s*begin\s*\(")

    def test_camera_examples_share_the_existing_display_i2c_bus(self):
        for name in ("Camera_Preview", "Camera_ISP_Tuning"):
            source = self.sources[name]
            with self.subTest(sketch=name):
                self.assertIn("DEV_I2C_Port port = DEV_I2C_Init();", source)
                self.assertIn("display_init(port)", source)
                self.assertIn("set_display_backlight(port, 255)", source)
                self.assertIn("cam_config.begin(port.bus", source)
                self.assertIn("CURRENT_SCREEN", source)
                self.assertIn('"displays_config.h"', source)
                self.assertNotRegex(source, r"CAMERA_SCCB|GPIO_NUM_[78]|Touch-LCD-5")

    def test_tuning_serial_poll_does_not_put_serial_in_a_while_condition(self):
        source = self.sources["Camera_ISP_Tuning"]
        self.assertIn("int available = Serial.available();", source)
        self.assertNotRegex(source, r"while\s*\([^)]*\bSerial\b[^)]*\)")
        self.assertIn("if (!applySetting(operation, value))", source)
        self.assertIn("if (!accepted) return false;", source)

    def test_sdmmc_uses_the_platform_four_bit_pinout(self):
        source = self.sources["SD_Card"]
        for name, pin in (("CLK", 43), ("CMD", 44), ("D0", 39), ("D1", 40), ("D2", 41), ("D3", 42)):
            self.assertRegex(source, rf"PLATFORM_SD_{name}\s*=\s*{pin}")
        self.assertIn("SD_MMC.setPins(PLATFORM_SD_CLK", source)
        self.assertIn("false /* four-bit mode */", source)

    def test_audio_uses_platform_es8311_playback_not_lcd5_input_audio(self):
        source = self.sources["Audio_Playback"]
        expected = {
            "PLATFORM_ES8311_ADDRESS": 0x18,
            "PLATFORM_AUDIO_PA": 53,
            "PLATFORM_I2S_MCLK": 13,
            "PLATFORM_I2S_BCLK": 12,
            "PLATFORM_I2S_LRCK": 10,
            "PLATFORM_I2S_DOUT": 9,
            "PLATFORM_I2C_PORT": 0,
        }
        for name, value in expected.items():
            self.assertRegex(source, rf"{name}\s*=\s*(?:0x{value:x}|{value})")
        self.assertIn(
            "es8311_create(PLATFORM_I2C_PORT, PLATFORM_ES8311_ADDRESS)", source
        )
        self.assertIn("if (!audio_ready)", source)
        self.assertLess(source.index("if (!audio_ready)"), source.index("playNote(note.frequency"))
        self.assertNotRegex(source, r"(?i)es7210|mic_record|touch-lcd-5")


if __name__ == "__main__":
    unittest.main()
