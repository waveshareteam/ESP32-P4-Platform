# Camera ISP Tuning

[中文版本](README_CN.md)

Exercise the OV5647 camera ISP tuning path on the Platform display.

## Requirements

- Arduino-ESP32 `3.3.11`, `ESP32P4 Dev Module`, PSRAM enabled, and default
  `ChipVariant=postv3` (v3.00+, 400 MHz).
- A supported OV5647 camera and the Platform display.
- The shared Platform I2C bus; do not use a separate camera-control bus.

## Build

Open `Camera_ISP_Tuning.ino` in Arduino IDE, or compile it with the documented
repository FQBN and board options.

## Expected Result

The sketch starts the camera/display path and exposes its ISP tuning behavior.
Compilation does not validate sensor tuning, captured image quality, or I2C
electrical behavior.

## HIL Checklist

Cold-start with the monitor closed, verify the expected tuning UI or preview,
then open UART0 at `115200` without resetting or stalling the application.

## Troubleshooting

Check the OV5647, PSRAM, display selection, and shared I2C bus. Pre-v3/v1.3
boards require a separate explicit `ChipVariant=prev3`/360 MHz build.
