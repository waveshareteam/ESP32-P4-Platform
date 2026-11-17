# Hello World

[中文版本](README_ZH.md)

Classic ESP-IDF hello world example for ESP32-P4.

This example prints chip information, flash size, and heap information, then
restarts after a countdown. If you are completely new to the board, run
[00_board_check](../00_board_check/) first because it keeps running and is
easier to watch in the serial monitor.

## Difficulty

Beginner.

## Hardware Required

- One ESP32-P4 board.
- USB cable.

No external peripheral is required.

## Build and Flash

```bash
cd examples/esp-idf/02_HelloWorld
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

Replace `PORT` with your serial port.

## Expected Output

The serial monitor should print `Hello world!`, chip information, flash size,
minimum heap size, and a restart countdown.

## Notes

- The example intentionally restarts after the countdown.
- Use this as a basic ESP-IDF smoke test after `00_board_check`.
