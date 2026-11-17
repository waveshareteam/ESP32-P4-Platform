# Wi-Fi Station

[中文版本](README_ZH.md)

Connect an ESP32-P4 setup to an existing Wi-Fi access point.

## Difficulty

Intermediate.

## Hardware Required

- A board or configuration with Wi-Fi support.
- A 2.4 GHz Wi-Fi access point.

Some ESP32-P4 boards require an external or companion Wi-Fi path. Check your
board documentation before using this example. In this project, the main
component manifest pulls in `esp_wifi_remote` and `esp_hosted`, so ESP32-P4
setups without native Wi-Fi must have the companion Wi-Fi transport configured
and connected before the station can associate.

## Build and Flash

Use an ESP-IDF terminal first. If your editor plugin fails to build this
example, verify the command-line flow before debugging the editor setup.

```bash
cd examples/esp-idf/10_wifistation
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

## Configuration

In `menuconfig`, open **Example Connection Configuration** or
**Example Configuration** and set:

- Wi-Fi SSID.
- Wi-Fi password.
- Maximum retry count, if needed.
- Accepted authentication mode. The default threshold is WPA2-PSK.

The source logs the configured SSID and password after a successful connection.
Avoid using production credentials in shared serial logs.

## Expected Output

The serial log should show Wi-Fi initialization, connection attempts, and an
IP address after DHCP succeeds.

## Troubleshooting

- Confirm the board has Wi-Fi support.
- Confirm the SSID and password.
- Use a 2.4 GHz network.
- Move the board closer to the access point if connection retries continue.
- If the build fails in an editor extension, retry from an ESP-IDF command-line
  shell so managed components and target-specific tools are initialized
  explicitly.
