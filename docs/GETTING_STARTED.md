# Getting Started

[中文版本](./GETTING_STARTED_CN.md)

This guide is written for first-time users. If you already know ESP-IDF or
Arduino, use [EXAMPLES_GUIDE.md](EXAMPLES_GUIDE.md) to jump directly to an
example.

## Choose Your Path

| You want to... | Start here |
| --- | --- |
| Confirm the board and toolchain work | `examples/esp-idf/00_board_check` |
| Learn the basic ESP-IDF build and monitor flow | `examples/esp-idf/02_HelloWorld` |
| Use a display from Arduino | `examples/arduino/examples/HelloWorld` |
| Bring up a hardware peripheral | Pick from [EXAMPLES_GUIDE.md](EXAMPLES_GUIDE.md) |
| Debug a build, flash, or boot problem | [TROUBLESHOOTING.md](TROUBLESHOOTING.md) |

## ESP-IDF First Run

1. Install an ESP-IDF version listed in [CI.md](CI.md). CI currently checks
   `v5.5.5` and `v6.0.2`; an example README may declare additional constraints.
2. Open an ESP-IDF terminal.
3. Clone this repository.
4. Build the board check example:

```bash
cd examples/esp-idf/00_board_check
idf.py set-target esp32p4
idf.py build
```

5. The default profile is `rev3_x`, using `MIN_300` for v3.x silicon. For
   pre-v3 silicon, including v1.3, explicitly select the legacy `rev1_3`
   profile, which uses `MIN_100` and `SELECTS_REV_LESS_V3`. See
   [ESP32-P4 Revision Config](ESP32P4_REVISION_CONFIG.md); do not reuse a
   generated `sdkconfig` or binary across these profiles.
6. Flash and open the monitor:

```bash
idf.py -p PORT flash monitor
```

Replace `PORT` with your serial port.

If you see the board check banner and periodic `alive` logs, your environment
is ready.

## Arduino First Run

1. Install Arduino IDE or arduino-cli.
2. Install a compatible Arduino-ESP32 core. See
   [examples/arduino/README.md](../examples/arduino/README.md).
3. Enable PSRAM and select `ChipVariant=postv3` (v3.00+, 400 MHz) in the
   Arduino board settings. Select `ChipVariant=prev3` (pre-v3, 360 MHz) only
   for a legacy pre-v3 board; its build output is not interchangeable.
4. Open `examples/arduino/examples/HelloWorld/HelloWorld.ino`.
5. Select the matching board and upload port.
6. Upload the sketch.

Most Arduino examples in this repository are display-oriented. They require a
compatible DSI display configuration and PSRAM. The same
[Arduino guide](../examples/arduino/README.md) documents the segmented CI
artifact, split-file flashing, and the required cold-start test with the serial
monitor closed. Opening a monitor is never a prerequisite for sketch startup.

## Beginner Checklist

- Use a data-capable USB cable, not a charge-only cable.
- Confirm the serial port appears when the board is connected.
- Start with `00_board_check` before examples that need Wi-Fi, Ethernet, SD
  card, display, touch, audio, or camera hardware.
- Use default `rev3_x`/`MIN_300` for v3.x chips. For v1.x chips, including
  v1.3, explicitly choose legacy `rev1_3`/`MIN_100`.
- Read the README inside the selected example directory.
- If an example has `menuconfig` settings, configure them before building.

## What to Read Next

- [EXAMPLES_GUIDE.md](EXAMPLES_GUIDE.md): choose an example by difficulty and
  peripheral.
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md): common setup and runtime problems.
- [PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md): repository layout for
  contributors.
