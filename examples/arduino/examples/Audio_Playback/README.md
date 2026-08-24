# Audio Playback

[中文版本](README_CN.md)

Play audio through the Platform ES8311 output codec and on-board speaker.

## Requirements

- Arduino-ESP32 `3.3.11`, `ESP32P4 Dev Module`, PSRAM enabled, and default
  `ChipVariant=postv3` (v3.00+, 400 MHz).
- The Platform ES8311 output codec and on-board speaker.

## Build

Open `Audio_Playback.ino` in Arduino IDE, or compile it with the documented
repository FQBN and board options.

## Expected Result

The sketch initializes playback for the Platform output path. It is not a
microphone example: LCD-5's ES7210 `Mic_Record` topology has not been ported to
this ES8311 product surface. Compilation does not prove audible playback.

## HIL Checklist

Cold-start with no serial monitor, verify audible playback through the on-board
speaker, then open UART0 at `115200` and confirm playback continues.

## Troubleshooting

Check that the selected board exposes the ES8311 and speaker path. Use a
separate explicit `ChipVariant=prev3`/360 MHz build for pre-v3/v1.3 silicon.
