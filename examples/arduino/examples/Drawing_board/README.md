# Drawing Board

[简体中文](README_CN.md)

Arduino drawing example using a DSI display and GT911 touch input.

## Difficulty

Intermediate.

## Hardware Required

- ESP32-P4 board.
- Supported DSI touch display.
- PSRAM enabled in the Arduino board settings.

## How to Run

1. Read [../../README.md](../../README.md).
2. Open `Drawing_board.ino`.
3. Select the correct board, port, and PSRAM setting.
4. Set `CURRENT_SCREEN` if needed.
5. Upload the sketch.

## Expected Behavior

The display shows a drawing surface. Touch input draws on the screen.

## Troubleshooting

- If the display works but touch does not, check the GT911 I2C path.
- Confirm the selected display configuration matches the physical screen.
