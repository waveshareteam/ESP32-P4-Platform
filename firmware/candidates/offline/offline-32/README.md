# offline-32 Firmware Candidates

[English](README.md) | [简体中文](README_ZH.md)

Boards covered by this profile are ESP32-P4-Pico. These ESP-IDF v6.1.1 candidates target 32 MB flash and ESP32-P4 rev3.x (v3.0–v3.99) for local hardware validation.

| Wi-Fi | Ethernet | Onboard audio | microSD | Xiaozhi |
| --- | --- | --- | --- | --- |
| No | No | Yes | Yes | No |

Select the firmware for the attached display and flash its merged BIN at `0x0`.

| Display | Resolution | File |
| --- | --- | --- |
| 7-DSI-TOUCH-A | 720 × 1280 | [ESP32-P4-Platform-offline-32-lcd7-a-rev3_x_FactoryOnly_20261118.bin](lcd7-a/ESP32-P4-Platform-offline-32-lcd7-a-rev3_x_FactoryOnly_20261118.bin) |
| 8-DSI-TOUCH-A | 800 × 1280 | [ESP32-P4-Platform-offline-32-lcd8-a-rev3_x_FactoryOnly_20261118.bin](lcd8-a/ESP32-P4-Platform-offline-32-lcd8-a-rev3_x_FactoryOnly_20261118.bin) |
| 10.1-DSI-TOUCH-A | 800 × 1280 | [ESP32-P4-Platform-offline-32-lcd10-1-a-rev3_x_FactoryOnly_20261118.bin](lcd10-1-a/ESP32-P4-Platform-offline-32-lcd10-1-a-rev3_x_FactoryOnly_20261118.bin) |

Set `PORT` to the board's serial port and run the matching display command from this directory.

For a clean factory restore, first run `esptool --chip esp32p4 --port "$PORT" erase-flash`. This clears settings and user data in flash.

## 7-DSI-TOUCH-A

```bash
esptool --chip esp32p4 --port "$PORT" write-flash 0x0 lcd7-a/ESP32-P4-Platform-offline-32-lcd7-a-rev3_x_FactoryOnly_20261118.bin
```

## 8-DSI-TOUCH-A

```bash
esptool --chip esp32p4 --port "$PORT" write-flash 0x0 lcd8-a/ESP32-P4-Platform-offline-32-lcd8-a-rev3_x_FactoryOnly_20261118.bin
```

## 10.1-DSI-TOUCH-A

```bash
esptool --chip esp32p4 --port "$PORT" write-flash 0x0 lcd10-1-a/ESP32-P4-Platform-offline-32-lcd10-1-a-rev3_x_FactoryOnly_20261118.bin
```

[Factory firmware source](../../../../examples/factory/).
