# How To Create Project

[中文版本](README_ZH.md)

This is the smallest ESP-IDF project layout in the repository. It is useful
when you want to understand which files are required before adding real
application logic.

## Difficulty

Beginner.

## Hardware Required

- One ESP32-P4 board.
- USB cable.

No external peripheral is required.

## Build

```bash
cd examples/esp-idf/01_HowToCreateProject
idf.py set-target esp32p4
idf.py build
```

This example has an empty `app_main()`, so it is mainly a project-structure
reference. For a more useful first run, use
[00_board_check](../00_board_check/).

## Files to Notice

- `CMakeLists.txt`: project entry point.
- `main/CMakeLists.txt`: registers the source file as an ESP-IDF component.
- `main/main.c`: application entry point.
