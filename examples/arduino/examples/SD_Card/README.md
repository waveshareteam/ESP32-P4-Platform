# SD Card

[中文版本](README_CN.md)

Access an inserted SD card with the Platform Arduino configuration.

## Requirements

- Arduino-ESP32 `3.3.11`, `ESP32P4 Dev Module`, PSRAM enabled, and default
  `ChipVariant=postv3` (v3.00+, 400 MHz).
- An inserted, supported SD card.

## Build

Open `SD_Card.ino` in Arduino IDE, or compile it with the documented repository
FQBN and board options.

## Expected Result

The sketch reports the card initialization and its basic storage operation.
Compilation does not prove card insertion, formatting, mounting, or data
integrity on a board.

## HIL Checklist

Cold-start with the serial monitor closed, verify the card operation, then open
UART0 at `115200` and confirm normal operation continues.

## Troubleshooting

Confirm that a supported card is inserted and correctly formatted. A pre-v3/v1.3
board needs an explicit, separate `ChipVariant=prev3`/360 MHz build.
