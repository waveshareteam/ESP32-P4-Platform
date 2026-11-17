# Ethernet Basic

[中文版本](README_ZH.md)

Bring up Ethernet, wait for link, and obtain an IP address.

## Difficulty

Intermediate.

## Hardware Required

- ESP32-P4 board with Ethernet support, or a supported external Ethernet
  module.
- Ethernet cable and network with DHCP.

## Build and Flash

```bash
cd examples/esp-idf/11_ethernetbasic
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

## Configuration

Check **Example Configuration** and the Ethernet PHY settings:

- `Stop and deinit Ethernet after elapsing number of secs`: leave at `-1` for a
  normal bring-up run, or set a value to exercise driver teardown.
- Internal EMAC or SPI Ethernet mode.
- PHY model and PHY address.
- MDC/MDIO GPIOs for internal EMAC.
- Reset GPIO.
- Clock mode.

The right values depend on the board schematic.

## Expected Output

The log should show Ethernet driver initialization, link up, the Ethernet MAC
address, and the DHCP result:

```text
Ethernet Started
Ethernet Link Up
Ethernet Got IP Address
ETHIP:...
```

Use this example before [15_eth2ap](../15_eth2ap/) because it validates the
Ethernet side without adding Wi-Fi AP forwarding.

## Troubleshooting

- Confirm your board actually has Ethernet hardware.
- Check cable, switch, and DHCP server.
- Verify PHY address and reset GPIO.
- If link never comes up, check clock mode and PHY power.
