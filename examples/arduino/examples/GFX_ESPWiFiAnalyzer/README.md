# GFX ESP Wi-Fi Analyzer

[简体中文](README_CN.md)

Arduino Wi-Fi scan visualization example using Arduino_GFX.

## Difficulty

Intermediate.

## Hardware Required

- ESP32-P4 board with Wi-Fi support.
- Supported DSI display.
- PSRAM enabled in the Arduino board settings.

## How to Run

1. Read [../../README.md](../../README.md).
2. Open `GFX_ESPWiFiAnalyzer.ino`.
3. Select the correct board, port, and PSRAM setting.
4. Set `CURRENT_SCREEN` if needed.
5. Upload the sketch.

## Expected Behavior

The display shows nearby Wi-Fi networks and their signal levels.

## Troubleshooting

- Confirm your board has Wi-Fi support.
- Confirm the display initializes with the simpler `HelloWorld` sketch first.
