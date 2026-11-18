# wifi-eth-16 Factory Firmware

[English](README.md) | [简体中文](README_ZH.md)

This profile applies to ESP32-P4-NANO, ESP32-P4-Module-DEV-KIT, and ESP32-P4-WIFI6-DEV-KIT (16 MB flash) with an ESP32-P4 rev3.x chip (v3.0 to v3.99). Images were built with ESP-IDF v6.1.1.

| Wi-Fi | Ethernet | Onboard audio | microSD | Xiaozhi |
| --- | --- | --- | --- | --- |
| Yes | Yes | Yes | Yes | Yes |

## Firmware downloads

Download the merged BIN matching the full model name of your attached display:

| Display | Resolution | File |
| --- | --- | --- |
| 7-DSI-TOUCH-A | 720 × 1280 | [ESP32-P4-Platform-7-DSI-TOUCH-A-wifi-eth-16-rev3-x_FactoryOnly_20261118.bin](ESP32-P4-Platform-7-DSI-TOUCH-A-wifi-eth-16-rev3-x_FactoryOnly_20261118.bin) |
| 8-DSI-TOUCH-A | 800 × 1280 | [ESP32-P4-Platform-8-DSI-TOUCH-A-wifi-eth-16-rev3-x_Factory_20261118.bin](ESP32-P4-Platform-8-DSI-TOUCH-A-wifi-eth-16-rev3-x_Factory_20261118.bin) |
| 10.1-DSI-TOUCH-A | 800 × 1280 | [ESP32-P4-Platform-10.1-DSI-TOUCH-A-wifi-eth-16-rev3-x_Factory_20261118.bin](ESP32-P4-Platform-10.1-DSI-TOUCH-A-wifi-eth-16-rev3-x_Factory_20261118.bin) |

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
esptool --chip esp32p4 --port "$PORT" write-flash 0x0 ESP32-P4-Platform-7-DSI-TOUCH-A-wifi-eth-16-rev3-x_FactoryOnly_20261118.bin
```

### 8-DSI-TOUCH-A

```bash
esptool --chip esp32p4 --port "$PORT" write-flash 0x0 ESP32-P4-Platform-8-DSI-TOUCH-A-wifi-eth-16-rev3-x_Factory_20261118.bin
```

### 10.1-DSI-TOUCH-A

```bash
esptool --chip esp32p4 --port "$PORT" write-flash 0x0 ESP32-P4-Platform-10.1-DSI-TOUCH-A-wifi-eth-16-rev3-x_Factory_20261118.bin
```

## Related links

- [Factory firmware source](../../examples/factory/README.md)
- [All factory firmware](../README.md)
