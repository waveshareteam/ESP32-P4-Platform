# Managed Components and Local Component Policy

[中文版本](COMPONENTS_CN.md)

This repository uses ESP-IDF Component Manager for reusable upstream
dependencies and keeps board- or example-specific glue beside the example that
owns it. Generated `managed_components/` directories and `dependencies.lock`
files are intentionally not committed.

## Managed Version Policy

First-party ESP32-P4 examples use the following constraints:

| Component | Constraint | Scope and reason |
| --- | --- | --- |
| `waveshare/esp32_p4_platform` | `2.0.1` | Exact board BSP version for examples 12, 13, 14, 16, and 18; update deliberately and validate both CI lanes |
| `esp_new_jpeg` | `1.0.2` | Exact JPEG helper version in example 17, avoiding an unbounded major-version change |
| `esp_video` | `>=2.2,<3.0` | Compatible 2.x API lane used by examples 16 and 17 |
| `espressif/esp_wifi_remote` | IDF 5.x: `0.14.*`; IDF 6.x: `>=1.6,<2.0` | Keeps the generated `esp_wifi` interface aligned with the selected ESP-IDF family |
| `espressif/esp_hosted` | IDF 5.x: `1.4.*`; IDF 6.x: `>=2.12,<3.0` | Direct dependency of examples 10 and 15; host/slave compatibility must also be checked on hardware |

Do not replace these constraints with `"*"`. When changing a version, update
this document and let the full ESP-IDF Actions matrix resolve every project on
ESP-IDF `v5.5.5` and `v6.0.2` before merging. Runtime-sensitive Hosted changes
also require the hardware checks in [P4_C6_HOSTED_WIFI.md](P4_C6_HOSTED_WIFI.md).

## Example-Local Components

The following directories are intentional local components, not stale
generated output:

| Path | Classification |
| --- | --- |
| `examples/esp-idf/09_sdmmc/components/sd_card` | SD-card example helper and test I/O glue owned by example 09 |
| `examples/esp-idf/11_ethernetbasic/components/ethernet_init` | Local Ethernet initialization helper; example 15 reuses it with `override_path` |
| `examples/esp-idf/12_I2SCodec` | No local support component; calls the managed `waveshare/esp32_p4_platform` and `esp_codec_dev` components directly |
| `examples/esp-idf/14_lvgl_demo_v9/components/lvgl_demo_support` | Product glue for the LVGL demo, including display behavior and local helpers where applicable |
| `examples/esp-idf/17_simple_video_server/components/example_video_common` | Shared video initialization/JPEG helper with target-conditional sources |
| `examples/esp-idf/18_esp_brookesia_phone/components` | Vendored Brookesia application stack used by the phone demo, including local override paths |

The former local BSP copies are removed. Reusable board support comes
from the online `waveshare/esp32_p4_platform` `2.0.1` managed component;
`lvgl_demo_support` and `product_support` remain local product glue, not a
replacement BSP. Keep such glue local until it has a stable public API,
independent tests, versioned release ownership, and more than one repository
consumer.

## CI Coverage Boundary

The default ESP-IDF matrix discovers the 20 direct projects under
`examples/esp-idf/` and builds them for target `esp32p4`. Target-specific files
such as the ESP32-C2 configuration in example 10 and the ESP32-S3 BSP
configuration in example 12 are retained, but they are not compiled by this
P4 product matrix. A green matrix therefore proves P4 compilation only; it is
not evidence for those additional targets or for hardware runtime behavior.

The maintained source project and immutable release packages under `firmware/`
are outside the example matrix and must follow a separate release process.
