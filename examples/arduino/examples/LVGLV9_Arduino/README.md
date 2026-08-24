# LVGL v9 Arduino

[简体中文](README_CN.md)

LVGL v9 demo for Arduino on supported ESP32-P4 display and touch hardware.

## Difficulty

Advanced.

## Hardware Required

- ESP32-P4 board.
- Supported DSI touch display.
- PSRAM enabled in the Arduino board settings.

## How to Run

1. Read [../../README.md](../../README.md).
2. Open `LVGLV9_Arduino.ino`.
3. Select the correct board, port, and PSRAM setting.
4. Set `CURRENT_SCREEN` if needed.
5. Upload the sketch.

## Expected Behavior

The display initializes and runs an LVGL demo with touch input.

## Troubleshooting

- Run Arduino `HelloWorld` first to verify the display path.
- Run `Drawing_board` first to verify touch input.
- Check LVGL memory and draw buffer settings if the board resets.
