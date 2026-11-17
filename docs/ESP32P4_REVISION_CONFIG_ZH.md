# ESP32-P4 修订版本配置

[English](ESP32P4_REVISION_CONFIG.md) | [简体中文](ESP32P4_REVISION_CONFIG_ZH.md)

默认配置面向 v3.x 芯片；pre-v3（包括 v1.3）必须显式选择 legacy 配置。

## 配置对应关系

| 配置 | ESP-IDF | Arduino Chip Variant |
| --- | --- | --- |
| rev3_x，默认 | `SELECTS_REV_LESS_V3=n`，`REV_MIN_300=y` | `postv3`，400 MHz |
| rev1_3，legacy | `SELECTS_REV_LESS_V3=y`，`REV_MIN_100=y` | `prev3`，360 MHz |

ESP-IDF 没有仅识别 v1.3 的专用选项；`MIN_100` 覆盖 v1.x 系列。不同配置的 `sdkconfig`、引导程序和应用固件须分别构建和保存。

## ESP-IDF 示例

在所选 `examples/esp-idf/<示例>/` 目录中，默认运行该示例 README 的命令即使用 rev3_x。pre-v3 芯片使用独立构建目录和配置：

```bash
idf.py -B build-rev1_3 -D SDKCONFIG=build-rev1_3/sdkconfig -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;../../../config/esp32p4_rev1_3.defaults" set-target esp32p4 build flash monitor
```

## 出厂源码

在 `examples/factory/` 目录中，默认配置选择 rev3_x；需要 pre-v3 时使用工程自带的覆盖文件：

```bash
idf.py -B build-rev1_3 -D SDKCONFIG=build-rev1_3/sdkconfig -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.rev1_3.defaults" set-target esp32p4 build flash monitor
```

## 构建覆盖

统一工作流按 `product.json` 构建 20 个 ESP-IDF 示例、9 个 Arduino 草图和一个出厂工程，均使用默认芯片配置。legacy 配置仍可在本地显式选择；自动构建不会按芯片版本扩展矩阵。
