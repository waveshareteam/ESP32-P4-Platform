# Camera Preview

[中文版本](README_CN.md)

Preview an OV5647 camera on the Platform display.

## Requirements

- Arduino-ESP32 `3.3.11`, `ESP32P4 Dev Module`, PSRAM enabled, and default
  `ChipVariant=postv3` (v3.00+, 400 MHz).
- A supported OV5647 camera and the Platform display.
- The shared Platform I2C bus; do not add a separate camera-control bus.

## Build

Open `Camera_Preview.ino` in Arduino IDE, or compile the sketch with the
repository's documented Arduino CLI FQBN and board options.

## Expected Result

The sketch initializes the supported camera and renders a preview. Compilation
only checks software configuration; it does not prove camera capture, display,
or I2C behavior on hardware.

## HIL Checklist

Power-cycle with every serial monitor closed, confirm preview starts, then open
UART0 at `115200` and confirm the application keeps running. Record board
revision, camera module, and result.

## Troubleshooting

Confirm the OV5647 module, PSRAM, display selection, and shared I2C path. For
pre-v3 silicon including v1.3, use explicit `ChipVariant=prev3`/360 MHz and do
not reuse the post-v3 build output.
