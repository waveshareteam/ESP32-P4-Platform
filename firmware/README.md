# Firmware

[中文版本](README_CN.md)

This directory contains ESP32-P4 Platform factory firmware sources and prebuilt factory firmware packages.

## Layout

- `brookesia/`: ESP-IDF firmware project.
- `factory_firmware/`: prebuilt factory firmware packages.

## Default Factory Firmware

The default prebuilt firmware is:

- `factory_firmware/ESP32-P4-Platform_Factory_Firmware_10.1-DSI-TOUCH-A_rev3.x-or-later`

It targets ESP32-P4 rev3.x or later hardware with 16 MB flash. A 7-DSI-TOUCH-A build is also provided for the existing 7-inch configuration.

## Build

Use ESP-IDF v5.5 or later:

```sh
cd firmware/brookesia
idf.py set-target esp32p4
idf.py build
```

The firmware project resolves LVGL and `waveshare/esp32_p4_platform` from the ESP-IDF Component Registry.

The example CI workflows do not build this maintained firmware project or
regenerate prebuilt factory packages. Treat existing BIN and ZIP files as
release artifacts; update them only through an explicit firmware release and
checksum review.

The separate firmware/Brookesia product jobs build rev1_3 and rev3_x and publish
temporary 32 MiB CI artifacts. Those profile-specific artifacts are not factory
firmware and do not change the immutable rev3.x-or-later 16 MiB packages checked
into factory_firmware/.
