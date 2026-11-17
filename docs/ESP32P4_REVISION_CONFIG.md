# ESP32-P4 Revision Configuration

[English](ESP32P4_REVISION_CONFIG.md) | [简体中文](ESP32P4_REVISION_CONFIG_ZH.md)

The default profile targets v3.x silicon. Pre-v3 devices, including v1.3, require an explicit legacy profile.

## Profile Mapping

| Profile | ESP-IDF | Arduino Chip Variant |
| --- | --- | --- |
| rev3_x, default | `SELECTS_REV_LESS_V3=n`, `REV_MIN_300=y` | `postv3`, 400 MHz |
| rev1_3, legacy | `SELECTS_REV_LESS_V3=y`, `REV_MIN_100=y` | `prev3`, 360 MHz |

ESP-IDF has no option that identifies only v1.3: `MIN_100` covers the v1.x family. Keep each profile's generated configuration, bootloader and application output separate.

## ESP-IDF Examples

Run the selected example's README commands in its `examples/esp-idf/<example>/` directory for the default rev3_x profile. For pre-v3 silicon, use a separate build directory and configuration:

```bash
idf.py -B build-rev1_3 -D SDKCONFIG=build-rev1_3/sdkconfig -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;../../../config/esp32p4_rev1_3.defaults" set-target esp32p4 build flash monitor
```

## Factory Source

In `examples/factory/`, the default profile is rev3_x. Use the project's own legacy override for pre-v3 silicon:

```bash
idf.py -B build-rev1_3 -D SDKCONFIG=build-rev1_3/sdkconfig -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.rev1_3.defaults" set-target esp32p4 build flash monitor
```

## Build Coverage

The shared workflow builds 20 ESP-IDF examples, nine Arduino sketches and one factory project using the versions in `product.json` and the default silicon profile. Legacy profiles remain available for explicit local builds; the automated matrix does not expand by chip revision.
