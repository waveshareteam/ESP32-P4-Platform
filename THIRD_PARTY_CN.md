# 第三方代码

[English](THIRD_PARTY.md)

本仓库包含或引用第三方代码，以便示例易于构建和评估。每个第三方组件仍受其自身许可证条款约束。

## 随附或示例本地代码

| 路径 | 项目或来源 | 说明 |
| --- | --- | --- |
| `examples/arduino/libraries/lvgl/` | LVGL | 为 Arduino 示例随附；请参阅该库自己的许可证文件。 |
| `examples/arduino/libraries/GFX_Library_for_Arduino/` | Arduino_GFX | 为 Arduino 显示示例随附；请参阅其元数据和许可证。 |
| `examples/arduino/libraries/displays/gt911.*` 和 `touch.*` | [Espressif LCD 触摸组件](https://github.com/espressif/esp-bsp/tree/442c9b3f21b3fe23dfe2e7470c14b7e74c956048/components/lcd_touch) | 按 Apache-2.0 改编至 Arduino；源码头与同目录 `NOTICE` 保留归属说明。`displays/` 中其余文件是受仓库许可证约束的 Waveshare 集成代码。 |
| `examples/arduino/examples/Audio_Playback/es8311*` | [Espressif ES8311 组件 `1.0.0~1`](https://github.com/espressif/esp-bsp/tree/4dabefca0599b2249d356cb6a08268dd5e56bbaf/components/es8311) | 按 Apache-2.0 改编至 Arduino-ESP32 I2C HAL；SPDX 文件头保留归属说明。 |
| `examples/esp-idf/18_esp_brookesia_phone/components/brookesia_core/` | ESP-Brookesia 核心相关代码 | 示例本地组件；请参阅随附文档和元数据。 |

## 托管组件

多个 ESP-IDF 示例使用 `idf_component.yml` 在构建期间从乐鑫组件注册表获取托管组件。生成的 `managed_components/` 目录和 `dependencies.lock` 文件属于本地构建输出，除非维护者有意固定特定依赖快照，否则不应提交。
在线 `waveshare/esp32_p4_platform` `2.0.1` 组件提供可复用 BSP 支持；
仓库本地的 `lvgl_demo_support` 和 `product_support` 是产品 glue，而不是随附的
BSP 副本。

## 维护者说明

添加或更新第三方代码时：

- 保留随附代码的上游许可证文件。
- 在本文件或示例 README 中记录项目名称、来源和版本。
- 当这样能让仓库更精简、更易更新时，优先使用 ESP-IDF 托管组件。
