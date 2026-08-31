# Board Check

[中文版本](./README_CN.md)

This is the recommended first ESP-IDF example for this repository.

It does not require any external module, display, camera, SD card, network, or
audio codec. It only prints board and runtime information through the serial
monitor, then keeps running so beginners can confirm that the toolchain,
flashing process, and serial monitor are working.

The default profile is `rev3_x`, which uses the `MIN_300` chip-revision target
for v3.x silicon. For pre-v3 silicon, including v1.3, explicitly select legacy
`rev1_3` (`MIN_100` and `SELECTS_REV_LESS_V3`); do not exchange generated
`sdkconfig` files or binaries between the profiles. Read
[../../../docs/ESP32P4_REVISION_CONFIG.md](../../../docs/ESP32P4_REVISION_CONFIG.md)
before flashing.

## What You Will Learn

- How to build and flash an ESP-IDF project.
- How to confirm the target is `esp32p4`.
- How to read chip, flash, PSRAM, and heap information from serial output.
- How to confirm the chip revision family before moving to peripheral demos.
- How to keep the serial monitor open and watch periodic logs.

## Hardware Required

- One supported ESP32-P4 board.
- USB cable for flashing and serial monitor.

No other hardware is required.

## Build and Flash

```bash
cd examples/esp-idf/00_board_check
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

Replace `PORT` with your serial port, for example `COMx` on Windows or
`/dev/ttyACMx` on Linux.

## Expected Output

The serial monitor should show output similar to:

```text
========================================
 ESP32-P4 Platform Board Check
========================================
IDF target: esp32p4
CPU cores: 2
Flash size: 16 MB
PSRAM: initialized
Board check is running. Open the serial monitor and confirm this output.
```

The example prints an `alive` log every five seconds. If you can see that log,
your build, flash, and monitor workflow is ready for the other examples.

## Next Steps

- Run [02_HelloWorld](../02_HelloWorld/) for the classic ESP-IDF hello world.
- Run [08_i2c_tools](../08_i2c_tools/) if you want to scan I2C devices.
- Read [../../../docs/EXAMPLES_GUIDE.md](../../../docs/EXAMPLES_GUIDE.md) to
  choose the next example for your board.
