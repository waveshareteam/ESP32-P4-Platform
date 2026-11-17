# Display Color Bar

[中文版本](README_ZH.md)

Display color bars on a supported LCD panel. This is the simplest ESP-IDF
display bring-up example in the repository.

## Difficulty

Intermediate.

## Hardware Required

- ESP32-P4 board.
- Supported LCD panel connected to the expected interface.

This example uses the `waveshare/esp32_p4_platform` BSP component. It is a good
first display test for boards supported by that BSP because the application does
not create an LVGL UI; it only initializes the panel and asks the DPI panel
driver to generate a hardware vertical color-bar pattern.

## Build and Flash

```bash
cd examples/esp-idf/13_Displaycolorbar
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

## Expected Behavior

The serial log should show:

```text
Initialize LCD device
Show color bar pattern drawn by hardware
```

The LCD should turn on its backlight and show vertical color bars.

## Troubleshooting

- Confirm the display panel model and interface.
- Confirm PSRAM is enabled when the panel path needs it.
- Check backlight control if logs look correct but the screen is dark.
- Verify reset, power, and panel initialization settings.
- Run this example before [14_lvgl_demo_v9](../14_lvgl_demo_v9/) or
  [16_video_lcd_display](../16_video_lcd_display/) so panel bring-up is tested
  separately from LVGL and camera processing.
