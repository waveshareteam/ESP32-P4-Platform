# ESP32-P4 + ESP32-C6 Hosted Wi-Fi

[中文版本](P4_C6_HOSTED_WIFI_CN.md)

ESP32-P4 has no native Wi-Fi radio. The Wi-Fi examples can use an ESP32-C6 as a
coprocessor through ESP-Hosted and expose the remote interface through
`esp_wifi_remote`. The P4 host components and the C6 slave firmware form one
compatibility unit; a successful P4 compile does not prove that an arbitrary C6
image will work.

## Repository Version Lanes

| Surface | Host stack | C6 firmware boundary |
| --- | --- | --- |
| ESP-IDF 5.x examples 10 and 15 | `esp_wifi_remote` `0.14.*`; `esp_hosted` `1.4.*` | Use the slave firmware required by the resolved Hosted release |
| ESP-IDF 6.x examples 10 and 15 | `esp_wifi_remote` `>=1.6,<2.0`; `esp_hosted` `>=2.12,<3.0` | Use the slave firmware required by the resolved Hosted release |
| ESP-IDF example 17 | Same direct `esp_wifi_remote` lanes; transport dependencies resolve through the selected component graph | Validate the resolved graph and slave image together |
| Arduino-ESP32 `3.3.11` | Use the dependencies shipped by the selected Arduino core | Do not override one Hosted component independently |

Do not infer compatibility from the highest available registry version. Match
the ESP-IDF family, `esp_wifi_remote`, ESP-Hosted host, and C6 slave firmware as
a tested set.

## What CI Proves

The ESP-IDF matrix proves that every direct P4 example compiles on the two
declared IDF lanes using default `rev3_x`/`MIN_300`. The Arduino workflow proves
nine-sketch compilation and segmented-package integrity using default
`ChipVariant=postv3` (v3.00+, 400 MHz). Pre-v3/v1.3 needs explicit
`rev1_3`/`MIN_100` or `prev3` (360 MHz); do not exchange the resulting build
outputs across revision families.
Neither workflow flashes a P4, flashes a C6, proves monitor-free cold startup,
negotiates the SDIO link, associates with an access point, or tests traffic.

## Hardware Acceptance

Before calling a Hosted change complete:

1. Flash the P4 image that matches the board revision.
2. Flash the C6 slave image required by that exact host stack.
3. Capture a fresh boot log and confirm Hosted SDIO initialization and C6
   enumeration without retry loops or reset storms.
4. Confirm association, DHCP, DNS, and bidirectional traffic.
5. Exercise disconnect/reconnect and a warm reboot.
6. Record board model/revision, P4 and C6 image versions, component versions,
   and the observed result. Sanitize credentials, unique identifiers, private
   network details, and local paths before sharing logs.
