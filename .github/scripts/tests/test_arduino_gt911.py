#!/usr/bin/env python3

import re
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[3]
GT911_HEADER = REPO_ROOT / "examples/arduino/libraries/displays/gt911.h"
GT911_SOURCE = REPO_ROOT / "examples/arduino/libraries/displays/gt911.cpp"
DRAWING_BOARD = REPO_ROOT / "examples/arduino/examples/Drawing_board/Drawing_board.ino"
LVGL = REPO_ROOT / "examples/arduino/examples/LVGLV9_Arduino/LVGLV9_Arduino.ino"


def function_body(source: str, signature: str) -> str:
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for index in range(brace, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[brace : index + 1]
    raise AssertionError(f"unterminated function: {signature}")


class ArduinoGt911Tests(unittest.TestCase):
    def setUp(self):
        self.header = GT911_HEADER.read_text(encoding="utf-8")
        self.source = GT911_SOURCE.read_text(encoding="utf-8")
        self.init = function_body(self.source, "esp_lcd_touch_handle_t touch_gt911_init")

    def test_probe_selects_the_actual_gt911_address_before_panel_io(self):
        primary_probe = self.init.index(
            "i2c_master_probe(port.bus, ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS, 100)"
        )
        backup_probe = self.init.index(
            "i2c_master_probe(port.bus, ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP, 100)"
        )
        panel_io = self.init.index("esp_lcd_new_panel_io_i2c")
        driver = self.init.index("esp_lcd_touch_new_i2c_gt911")

        self.assertLess(primary_probe, backup_probe)
        self.assertLess(backup_probe, panel_io)
        self.assertLess(panel_io, driver)
        self.assertIn("ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG_WITH_ADDRESS(address)", self.init)
        self.assertIn(".dev_addr = (_address)", self.header)

    def test_touch_pins_remain_nc_and_initialization_stays_polling_only(self):
        self.assertRegex(
            self.header,
            r"#define EXAMPLE_PIN_NUM_TOUCH_RST\s+\(GPIO_NUM_NC\)",
        )
        self.assertRegex(
            self.header,
            r"#define EXAMPLE_PIN_NUM_TOUCH_INT\s+\(GPIO_NUM_NC\)",
        )
        self.assertIn(".rst_gpio_num = EXAMPLE_PIN_NUM_TOUCH_RST", self.init)
        self.assertIn(".int_gpio_num = EXAMPLE_PIN_NUM_TOUCH_INT", self.init)
        self.assertNotIn("esp_lcd_touch_register_interrupt_callback", self.init)
        self.assertNotIn("gpio_isr_handler_add", self.init)

    def test_init_fails_closed_and_releases_panel_io_after_driver_failure(self):
        self.assertIn("if (port.bus == NULL)", self.init)
        self.assertIn("GT911 not found", self.init)
        self.assertGreaterEqual(self.init.count("return NULL;"), 4)
        failure = self.init.index("if (ret != ESP_OK)", self.init.index("esp_lcd_touch_new_i2c_gt911"))
        cleanup = self.init.index("esp_lcd_panel_io_del(tp_io_handle);", failure)
        return_null = self.init.index("return NULL;", cleanup)
        self.assertLess(failure, cleanup)
        self.assertLess(cleanup, return_null)
        self.assertNotIn("ESP_ERROR_CHECK", self.init)

    def test_examples_continue_without_a_touch_handle(self):
        drawing = DRAWING_BOARD.read_text(encoding="utf-8")
        lvgl = LVGL.read_text(encoding="utf-8")

        drawing_loop = function_body(drawing, "void loop()")
        self.assertIn("if (tp_handle != NULL)", drawing_loop)
        self.assertLess(
            drawing_loop.index("if (tp_handle != NULL)"),
            drawing_loop.index("esp_lcd_touch_read_data(tp_handle)"),
        )

        read_callback = function_body(lvgl, "void my_touchpad_read")
        self.assertIn("if (tp_handle == NULL)", read_callback)
        self.assertIn("data->state = LV_INDEV_STATE_RELEASED;", read_callback)
        self.assertLess(
            read_callback.index("if (tp_handle == NULL)"),
            read_callback.index("esp_lcd_touch_read_data(tp_handle)"),
        )

    def test_shared_point_reader_is_safe_without_a_touch_handle(self):
        reader = function_body(self.source, "touch_gt911_point_t touch_gt911_read_point")
        guard = reader.index("if (tp_handle == NULL || max_touch_cnt == 0)")
        read = reader.index("esp_lcd_touch_read_data(tp_handle)")
        self.assertLess(guard, read)
        self.assertIn("touch_gt911_point_t data = {};", reader)


if __name__ == "__main__":
    unittest.main()
