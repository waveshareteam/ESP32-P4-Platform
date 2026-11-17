# Ethernet To AP

[中文版本](README_ZH.md)

Network example that combines Ethernet and Wi-Fi AP behavior.

## Difficulty

Advanced.

## Hardware Required

- Ethernet-capable ESP32-P4 board or supported Ethernet module.
- Wi-Fi support path.
- Ethernet cable and network.

This example forwards packets between Ethernet and a Wi-Fi SoftAP. On ESP32-P4
setups in this repository, the project uses `esp_wifi_remote` and `esp_hosted`
for the Wi-Fi path, so the companion Wi-Fi transport must be available before
the AP can pass traffic.

## Build and Flash

Use an ESP-IDF terminal first. If your editor plugin fails to build this
example, verify the command-line flow before debugging the editor setup.

```bash
cd examples/esp-idf/15_eth2ap
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

## Configuration

Check both Ethernet and Wi-Fi AP settings in `menuconfig`.

Default AP settings in this project:

| Setting | Default |
| --- | --- |
| SSID | `ESP32-P4-NANO-WiFi` |
| Password | `12345678` |
| Channel | `1` |
| Maximum stations | `4` |

The default ESP32-P4 Ethernet pins in `sdkconfig.defaults` are:

| Signal | GPIO |
| --- | --- |
| MDC | GPIO31 |
| MDIO | GPIO52 |
| PHY reset | GPIO51 |

The correct Ethernet PHY model, address, reset, clock, and SMI values still
depend on the board schematic.

## Expected Behavior

When the Ethernet link comes up, the example copies the Ethernet MAC address to
the SoftAP interface and starts Wi-Fi AP mode. When the Ethernet link goes
down, it stops the AP. Client packets are forwarded through an internal flow
control queue to balance Ethernet and Wi-Fi throughput.

## Troubleshooting

- Bring up [11_ethernetbasic](../11_ethernetbasic/) first.
- Bring up [10_wifistation](../10_wifistation/) or another Wi-Fi example first
  to verify the Wi-Fi transport.
- Check IP address ranges and DHCP behavior.
- Confirm your board supports both required network paths.
- If the build fails in an editor extension, retry from an ESP-IDF command-line
  shell so managed components and target-specific tools are initialized
  explicitly.
