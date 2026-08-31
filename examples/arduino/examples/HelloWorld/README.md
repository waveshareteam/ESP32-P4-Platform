# Arduino Hello World

[简体中文](README_CN.md)

Minimal Arduino display test for supported ESP32-P4 DSI display setups.

## Difficulty

Beginner.

## Hardware Required

- ESP32-P4 board.
- Supported DSI display.
- PSRAM enabled in the Arduino board settings.

## How to Run

1. Read [../../README.md](../../README.md) for Arduino core and library notes.
2. Open `HelloWorld.ino` in Arduino IDE.
3. Select the correct ESP32-P4 board and port.
4. Enable PSRAM.
5. Set `CURRENT_SCREEN` if your display is not the default.
6. Upload the sketch.

## Expected Behavior

The screen initializes, turns on the backlight, and prints `Hello World!`.
