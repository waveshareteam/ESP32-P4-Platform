# LVGL Demo v9

[中文版本](./README_CN.md)

Run LVGL v9 demos on a supported display.

## Difficulty

Advanced.

## Hardware Required

- ESP32-P4 board.
- Supported LCD panel.
- PSRAM enabled.

Touch hardware may be required depending on the selected LVGL demo and board
configuration.

## Build and Flash

```bash
cd examples/esp-idf/14_lvgl_demo_v9
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

## Configuration

This example uses LVGL v9 and the online
`waveshare/esp32_p4_platform` `2.0.1` managed BSP component.
`components/lvgl_demo_support` is local product glue, not a BSP copy. Check
`main/idf_component.yml` and `sdkconfig.defaults` when changing LVGL or
display behavior.

The current application starts the BSP display with:

- Rotation `ESP_LV_ADAPTER_ROTATE_0`.
- Triple partial anti-tearing mode.
- No touch axis swap or mirroring.
- `lv_demo_widgets()` enabled by default.

Other LVGL demos such as `lv_demo_music()` and `lv_demo_benchmark()` are already
enabled in `sdkconfig.defaults` and can be selected by changing the call in
`main/main.c`.

## Expected Behavior

The display should initialize, turn on the backlight, and run the LVGL widgets
demo with the LVGL performance monitor enabled.

## Troubleshooting

- Run [13_Displaycolorbar](../13_Displaycolorbar/) first to verify the panel.
- Confirm PSRAM is initialized.
- Check LVGL buffer size and display resolution settings.
- If touch does not work, verify the touch controller and I2C pins separately.
- If you change anti-tearing mode while using ESP-IDF 5.5, check the note in
  `main/main.c` about the three-buffer anti-tearing fix.
