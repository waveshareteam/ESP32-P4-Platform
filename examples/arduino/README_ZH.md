# ESP32-P4 Arduino 示例

[English](README.md) | [简体中文](README_ZH.md)

使用随仓库提供的库运行这些草图，版本和完整 FQBN 以 [product.json](../../.github/product.json) 为准。

## 环境设置

1. 安装 Arduino IDE 和 Arduino-ESP32 `3.3.11`。
2. 将项目文件夹位置设为 `examples/arduino/`，或将 `libraries/` 复制到现有项目文件夹的 `libraries` 目录。
3. 选择 `ESP32P4 Dev Module`，核对以下选项后上传草图。

| 工具菜单 | 配置 |
| --- | --- |
| PSRAM | Enabled |
| USB Mode | USB-OTG (TinyUSB)，即 `USBMode=default` |
| Chip Variant | post-v3 / v3.00+，即 `ChipVariant=postv3` |
| Flash Size | 16 MB |
| Partition Scheme | 3MB APP/9.9MB FATFS，16 MB |

较早芯片须显式选择 `ChipVariant=prev3`，构建产物与 post-v3 不可互换，参见[修订版本指南](../../docs/ESP32P4_REVISION_CONFIG_ZH.md)。显示示例通过 `CURRENT_SCREEN` 选择 10.1、8 或 7 英寸 DSI 屏，默认是 10.1 英寸。

## 随仓库提供的库

- `board_config/`：板级显示、I2C 与 GT911 触摸封装。
- `GFX_Library_for_Arduino/`：Arduino_GFX `1.6.0`。
- `lvgl/` 和 `lv_conf.h`：LVGL `9.3.0` 及配套配置。
- `Waveshare_Arduino_Logging/`：草图共用的串口日志初始化。

保留这些库及其许可证，避免额外安装的同名库覆盖当前配置；来源见[第三方代码说明](../../docs/THIRD_PARTY_ZH.md)。GT911 采用轮询方式，依次探测 `0x5D` 和 `0x14`，不指定 INT/RST GPIO；没有触摸设备时草图继续运行。

## 串口与启动

当前配置未启用 USB CDC On Boot，日志使用 UART0；请选择实际板卡文档标出的 UART 编程接口，波特率为 `115200`。草图启动不需要打开串口监视器。

关闭监视器后断电重启，确认正常功能已启动，再打开和关闭监视器检查程序是否持续运行。Wi-Fi 示例需要与主机组件匹配的 C6 从机固件，参见 [P4 + C6 说明](../../docs/P4_C6_HOSTED_WIFI_ZH.md)。

## 示例

示例列表与硬件需求见[仓库首页](../../README_ZH.md)和各示例的 `README_ZH.md`。Camera 示例使用 OV5647 并复用显示屏 I2C 总线；SD_Card 需要 FAT32 存储卡，Audio_Playback 使用 ES8311 和扬声器。

## 固件包

示例固件由统一 [Build 工作流](../../.github/workflows/build.yml)生成，包内提供烧录脚本和说明，格式见[模板固件文档](https://github.com/waveshareteam/esp32-repo-template/blob/main/docs/release.md)。
