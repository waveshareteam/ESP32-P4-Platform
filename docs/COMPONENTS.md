# Managed Components and Local Component Policy

[中文版本](COMPONENTS_ZH.md)

This repository uses ESP-IDF Component Manager for reusable upstream
dependencies and keeps board- or example-specific glue beside the example that
owns it. Generated `managed_components/` directories and `dependencies.lock`
files are intentionally not committed.

## Managed Version Policy

First-party ESP32-P4 examples use the following constraints:

| Component | Constraint | Scope and reason |
| --- | --- | --- |
| `waveshare/esp32_p4_platform` | `2.0.1` | Exact board BSP version for examples 12, 13, 14, 16, and 18; update deliberately and validate the configured ESP-IDF version |
| `lvgl/lvgl` | `>=9.3,<9.6` | ESP32-P4 examples 12, 13, 14 and 16 on ESP-IDF 6.1: BSP 2.0.1 uses the 0.6.x adapter, whose legacy headers fail with LVGL 9.6 under IDF 6; remove the cap after adopting a BSP adapter that supports the new headers |
| `esp_new_jpeg` | `1.0.2` | Exact JPEG helper version in example 17, avoiding an unbounded major-version change |
| `esp_video` | `>=2.2,<3.0` | Compatible 2.x API used by examples 16 and 17 and the factory camera |
| `chmorgan/esp-audio-player` | `^1.1.0` | Factory audio and the LVGL demo: declares the split I2S driver dependency required by IDF 6 |
| `chmorgan/esp-file-iterator` | `1.0.0` | Factory playlist; product glue releases the iterator's owned allocations because this version declares its deletion function without implementing it |
| `espressif/esp_wifi_remote` | `>=1.6,<2.0` | Keeps the generated `esp_wifi` interface aligned with the selected ESP-IDF family |
| `espressif/esp_hosted` | `>=2.12,<3.0` | Direct dependency of examples 10 and 15; host/slave compatibility must also be checked on hardware |

Do not replace these constraints with `"*"`. When changing a version, update
this document and let the full ESP-IDF Actions matrix resolve every project on
ESP-IDF `v6.1.1` before merging. Runtime-sensitive Hosted changes
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
| `examples/factory/components/factory_ui` | Shared LVGL portrait views; the dark-card and blue-control style follows the [Knob factory UI](https://github.com/waveshareteam/ESP32-P4-Knob-Touch-AMOLED-1.75/tree/7671170de4f5fe04ae9c9a9273f6b6119eb7fb0b/examples/factory), with rectangular 800 x 1280 geometry |
| `examples/factory/components/json` | Bridges Brookesia's json dependency to the managed IDF 6 cJSON component |

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

The factory source project in `examples/factory/` is built separately with
`factory_firmware.esp_idf` from `product.json`; it is outside the example matrix.

The factory project follows the reference build by compiling its Boost component as C++23; other components retain the ESP-IDF 6.1 default. This avoids C++26 deprecation errors in the older Boost headers while keeping warning errors enabled.
