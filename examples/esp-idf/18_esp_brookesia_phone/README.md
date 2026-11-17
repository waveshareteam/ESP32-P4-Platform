# ESP32-P4 Platform Running ESP-Brookesia Phone Example

[中文版本](README_ZH.md)

This example demonstrates how to run the ESP-Brookesia Phone UI on the
Waveshare ESP32-P4 Platform display BSP. It is adapted from Espressif's
ESP-Brookesia phone example for the ESP32-P4 Platform board and the supported
Waveshare DSI touch displays in this repository.

The project uses the ESP-Brookesia and BSP components already included in this
example directory, starts the BSP display, installs the phone apps from the
registry, and updates the status-bar clock with an LVGL timer.

## Getting Started

### Hardware Requirements

* An ESP32-P4 Platform board supported by `waveshare/esp32_p4_platform`.
* A supported Waveshare DSI touch display selected in the BSP menu.
* PSRAM enabled.

### ESP-IDF Required

- This example supports IDF release/v5.3 and later branches.
- Please follow the [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html) to set up the development environment. **We highly recommend** you [Build Your First Project](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#build-your-first-project) to get familiar with ESP-IDF and make sure the environment is set up correctly.

### Included Components

No separate `esp-brookesia` clone is required for this repository example. The application uses:

- `components/brookesia_core`
- `components/brookesia_app_squareline_demo`
- `waveshare/esp32_p4_platform` from the component manifest

### Configuration

Run `idf.py menuconfig` and modify the ESP-Brookesia and BSP configuration.

The default `sdkconfig.defaults` disables AI framework, services, speaker
system, and some Boost modules to keep this phone UI demo focused on display
and touch. It also selects `CONFIG_BSP_LCD_TYPE_720_1280_7_INCH_A`, enables
LVGL logging, and uses three LCD DPI buffers.

Select the actual display connected to your board in:

```text
Board Support Package(ESP32-P4) -> Display -> Select LCD type
```

At runtime, the application selects a dark stylesheet for `1024 x 600` or
`800 x 1280` displays when the BSP resolution matches those values.

## How to Use the Example

### Build and Flash the Example

Build the project and flash it to the board, then run monitor tool to view serial output (replace `PORT` with your board's serial port name):

```bash
idf.py set-target esp32p4
idf.py -p PORT flash monitor
```

To exit the serial monitor, type `Ctrl-]`.

See the [ESP-IDF Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html) for full steps to configure and use ESP-IDF to build projects.

### Expected Behavior

The log prints `Display ESP-Brookesia phone demo`, the display backlight turns
on, and the phone UI appears. The example starts the BSP display with rotation
`ESP_LV_ADAPTER_ROTATE_0`, triple partial anti-tearing mode, and default touch
axis settings.

### Troubleshooting

- Run [13_Displaycolorbar](../13_Displaycolorbar/) first to verify the LCD path.
- Run [14_lvgl_demo_v9](../14_lvgl_demo_v9/) if you need to isolate LVGL/BSP
  display issues before debugging Brookesia.
- If the UI appears but touch is wrong, check the BSP touch controller and
  `touch_flags` in `main/main.cpp`.
- If you change anti-tearing mode while using ESP-IDF 5.5, check the note in
  `main/main.cpp` about the three-buffer anti-tearing fix.

## Technical Support and Feedback

Please use the following feedback channels:

- For technical queries, go to the [esp32.com](https://esp32.com/viewforum.php?f=22) forum.
- For a feature request or bug report, create a [GitHub issue](https://github.com/espressif/esp-brookesia/issues).

We will get back to you as soon as possible.
