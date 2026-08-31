# Example Roadmap

[中文版本](./EXAMPLE_ROADMAP_CN.md)

This roadmap tracks example coverage that would make the repository more
complete for both beginners and advanced users.

## Current Strengths

- ESP-IDF project basics.
- I2C command-line tools.
- NVS persistent settings.
- FreeRTOS task and queue basics.
- UART loopback.
- GPIO input/output basics.
- GPIO interrupt and debounce.
- Wi-Fi station and Ethernet examples.
- SDMMC card example.
- I2S codec example.
- Display, LVGL, camera, video server, and ESP-Brookesia examples.
- Arduino display, touch, camera, SD-card, and ES8311 playback examples with
  bundled libraries where required.

## High-Priority Additions

| Priority | Example | Audience | Why it matters |
| --- | --- | --- | --- |
| High | Display backlight and color fill | Beginner | Easier than full LVGL for display bring-up |
| High | Touch coordinate logger | Beginner to intermediate | Separates touch bring-up from drawing/UI code |
| High | UART external module template | Intermediate | Builds on loopback for real device integration |

## Medium-Priority Additions

| Priority | Example | Audience | Why it matters |
| --- | --- | --- | --- |
| Medium | ADC periodic sampling | Beginner | Teaches analog input and timers |
| Medium | SPI device template | Intermediate | Helps users add sensors and modules |
| Medium | Wi-Fi SoftAP | Intermediate | Useful when no router is available |
| Medium | HTTP client | Intermediate | Common IoT workflow |
| Medium | SPIFFS or LittleFS basics | Intermediate | Filesystem workflow without SD hardware |
| Medium | Power management | Advanced | Sleep and wake-up patterns for products |

## Documentation Work

- Add board-specific notes for pin mappings and supported add-on modules.
- Add expected serial output snippets to simple examples.
- Add troubleshooting sections to display, touch, camera, and network examples.

## Quality Bar for New Examples

Every new example should include:

- A focused goal.
- Required hardware.
- Supported boards or known board assumptions.
- Build and flash commands.
- `menuconfig` options, if any.
- Expected serial output or visible behavior.
- Known limitations.
