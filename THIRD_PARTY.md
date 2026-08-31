# Third-Party Code

[简体中文](THIRD_PARTY_CN.md)

This repository includes or references third-party code so the examples are
easy to build and evaluate. Each third-party component remains governed by its
own license terms.

## Bundled or Example-Local Code

| Path | Project or source | Notes |
| --- | --- | --- |
| `examples/arduino/libraries/lvgl/` | LVGL | Bundled for Arduino examples; see the library's own license file. |
| `examples/arduino/libraries/GFX_Library_for_Arduino/` | Arduino_GFX | Bundled for Arduino display examples; see its metadata and license. |
| `examples/arduino/libraries/displays/gt911.*` and `touch.*` | [Espressif LCD touch components](https://github.com/espressif/esp-bsp/tree/442c9b3f21b3fe23dfe2e7470c14b7e74c956048/components/lcd_touch) | Arduino-adapted under Apache-2.0; source headers and the nearby `NOTICE` retain attribution. The other files in `displays/` are Waveshare integration code covered by the repository license. |
| `examples/arduino/examples/Audio_Playback/es8311*` | [Espressif ES8311 component `1.0.0~1`](https://github.com/espressif/esp-bsp/tree/4dabefca0599b2249d356cb6a08268dd5e56bbaf/components/es8311) | Apache-2.0 codec source adapted to the Arduino-ESP32 I2C HAL; SPDX headers retain attribution. |
| `examples/esp-idf/18_esp_brookesia_phone/components/brookesia_core/` | ESP-Brookesia core-related code | Example-local component; see included documentation and metadata. |

## Managed Components

Several ESP-IDF examples use `idf_component.yml` to fetch managed components
from Espressif's component registry during build. Generated
`managed_components/` directories and `dependencies.lock` files are local build
outputs and should not be committed unless maintainers intentionally pin a
specific dependency snapshot. The online
`waveshare/esp32_p4_platform` `2.0.1` component provides reusable BSP
support; repository-local `lvgl_demo_support` and `product_support` are
product glue rather than bundled BSP copies.

## Maintainer Notes

When adding or updating third-party code:

- Keep the upstream license file with the bundled code.
- Record the project name, source, and version in this file or the example
  README.
- Prefer ESP-IDF managed components when that keeps the repository smaller and
  easier to update.
