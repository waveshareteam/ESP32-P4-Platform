# System Monitor

[中文版本](README_ZH.md)

Serial diagnostics and runtime monitor for advanced users.

This demo is useful as a product bring-up or field-debug starting point. It
combines several patterns from earlier examples:

- NVS initialization and persistent settings.
- Default ESP-IDF event loop.
- FreeRTOS queue and worker tasks.
- Periodic heap and PSRAM sampling.
- Interactive serial console commands.

## Difficulty

Advanced.

## Hardware Required

- One ESP32-P4 board.
- USB cable.

No external peripheral is required.

## Build and Flash

```bash
cd examples/esp-idf/19_system_monitor
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

## Expected Output

The monitor prints periodic runtime samples:

```text
sample=0 uptime=1234ms heap=123456 min_heap=120000 psram=8123456 period=2000ms
```

The serial console prompt is:

```text
monitor>
```

## Console Commands

| Command | Purpose |
| --- | --- |
| `help` | List available commands |
| `info` | Print target, chip revision, flash, PSRAM, and sampling settings |
| `heap` | Print current heap and PSRAM usage |
| `period <ms>` | Save a new sampling interval to NVS |

Example:

```text
monitor> period 1000
sample period saved: 1000 ms
```

## Why This Example Exists

This is a practical application shell for real projects. You can extend it with
Wi-Fi, Ethernet, display, storage, sensors, or production test commands while
keeping one consistent serial diagnostics entry point.
