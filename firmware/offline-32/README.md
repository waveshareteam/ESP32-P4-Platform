# offline-32 Factory Firmware

[English](README.md) | [简体中文](README_ZH.md)

This profile applies to ESP32-P4-Pico (32 MB flash) with an ESP32-P4 rev3.x chip (v3.0 to v3.99). Images were built with ESP-IDF v6.1.1.

| Wi-Fi | Ethernet | Onboard audio | microSD | Xiaozhi |
| --- | --- | --- | --- | --- |
| No | No | Yes | Yes | No |

## Firmware downloads

Download the merged BIN matching the full model name of your attached display:

| Display | Resolution | File |
| --- | --- | --- |
| 7-DSI-TOUCH-A | 720 × 1280 | [ESP32-P4-Platform-7-DSI-TOUCH-A-offline-32-rev3-x_FactoryOnly_20261125.bin](ESP32-P4-Platform-7-DSI-TOUCH-A-offline-32-rev3-x_FactoryOnly_20261125.bin) |
| 8-DSI-TOUCH-A | 800 × 1280 | [ESP32-P4-Platform-8-DSI-TOUCH-A-offline-32-rev3-x_Factory_20261125.bin](ESP32-P4-Platform-8-DSI-TOUCH-A-offline-32-rev3-x_Factory_20261125.bin) |
| 10.1-DSI-TOUCH-A | 800 × 1280 | [ESP32-P4-Platform-10.1-DSI-TOUCH-A-offline-32-rev3-x_Factory_20261125.bin](ESP32-P4-Platform-10.1-DSI-TOUCH-A-offline-32-rev3-x_Factory_20261125.bin) |

## Flashing

Flashing or erasing overwrites existing applications, settings, and user data in flash. Back up data as needed before proceeding.

1. In Bash, change to the directory containing the BIN file.
2. Set `PORT` to the board's serial port:

   ```bash
   PORT='<serial-port>'
   ```

3. (Optional) To perform a clean factory restore, erase the entire flash:

   ```bash
   esptool --chip esp32p4 --port "$PORT" erase-flash
   ```

4. In the same Bash session, flash the image for your display at `0x0`:

### 7-DSI-TOUCH-A

```bash
esptool --chip esp32p4 --port "$PORT" write-flash 0x0 ESP32-P4-Platform-7-DSI-TOUCH-A-offline-32-rev3-x_FactoryOnly_20261125.bin
```

### 8-DSI-TOUCH-A

```bash
esptool --chip esp32p4 --port "$PORT" write-flash 0x0 ESP32-P4-Platform-8-DSI-TOUCH-A-offline-32-rev3-x_Factory_20261125.bin
```

### 10.1-DSI-TOUCH-A

```bash
esptool --chip esp32p4 --port "$PORT" write-flash 0x0 ESP32-P4-Platform-10.1-DSI-TOUCH-A-offline-32-rev3-x_Factory_20261125.bin
```

After flashing the `20261125` image, open the board’s USB-to-UART port at **115200 baud (8N1)** and reset the board to read bootloader and application logs.

## Related links

- [Factory firmware source](../../examples/factory/README.md)
- [All factory firmware](../README.md)
