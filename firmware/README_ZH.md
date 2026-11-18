# 出厂固件

[English](README.md) | [简体中文](README_ZH.md)

本目录提供 ESP32-P4 系列开发板的出厂固件及烧录说明。[固件清单](manifest.json) 记录了各板卡对应的说明文档与固件文件。

| 板卡 | 网络 | Flash | 出厂配置 | 固件下载 |
| --- | --- | ---: | --- | --- |
| [ESP32-P4-NANO](https://docs.waveshare.com/ESP32-P4-NANO) | Wi-Fi + Ethernet | 16 MB | `wifi-eth-16.defaults` | [wifi-eth-16](wifi-eth-16/README_ZH.md) |
| [ESP32-P4-Module-DEV-KIT](https://docs.waveshare.com/ESP32-P4-Module-DEV-KIT) | Wi-Fi + Ethernet | 16 MB | `wifi-eth-16.defaults` | [wifi-eth-16](wifi-eth-16/README_ZH.md) |
| [ESP32-P4-WIFI6-DEV-KIT](https://docs.waveshare.com/ESP32-P4-WIFI6-DEV-KIT) | Wi-Fi + Ethernet | 16 MB | `wifi-eth-16.defaults` | [wifi-eth-16](wifi-eth-16/README_ZH.md) |
| [ESP32-P4-WIFI6-POE-ETH](https://docs.waveshare.com/ESP32-P4-WIFI6-POE-ETH) | Wi-Fi + Ethernet | 32 MB | `wifi-eth-32.defaults` | [wifi-eth-32](wifi-eth-32/README_ZH.md) |
| [ESP32-P4-WIFI6](https://docs.waveshare.com/ESP32-P4-WIFI6) | Wi-Fi | 32 MB | `wifi-32.defaults` | [wifi-32](wifi-32/README_ZH.md) |
| [ESP32-P4-ETH](https://docs.waveshare.com/ESP32-P4-ETH) | Ethernet | 32 MB | `eth-32.defaults` | [eth-32](eth-32/README_ZH.md) |
| [ESP32-P4-Pico](https://docs.waveshare.com/ESP32-P4-Pico) | 无板载网络 | 32 MB | `offline-32.defaults` | [offline-32](offline-32/README_ZH.md) |
| [ESP32-P4-Core-DEV-KIT](https://docs.waveshare.com/ESP32-P4-Core-DEV-KIT) | 无板载网络 | 32 MB | `offline-32.defaults` + `core-dev-kit.defaults` | [core-dev-kit](core-dev-kit/README_ZH.md)（无板载音频和 microSD） |

## 选型与烧录

- **核对硬件参数**：下载前请核对板卡型号、芯片修订版、Flash 容量和屏幕型号。上表配置仅面向 rev3.x 芯片，同名板卡可能存在其他芯片修订版。
- **区分屏幕型号**：请按文件名中的完整屏幕型号选择 BIN。`7-DSI-TOUCH-A` 分辨率为 720 × 1280；`8-DSI-TOUCH-A` 和 `10.1-DSI-TOUCH-A` 分辨率均为 800 × 1280，但初始化配置不同。
- **备份后烧录**：烧录合并 BIN 会覆盖 Flash 中原有的应用和设置，操作前请先按需备份。合并 BIN 须从 `0x0` 烧录，各板卡的固件文件和烧录命令见上表「固件下载」列。

## 从源码构建

源码及构建示例见[出厂工程构建说明](../examples/factory/README_ZH.md)。在 `examples/factory/` 目录下，按顺序组合 `sdkconfig.defaults`、`sdkconfig.rev3_x.defaults`、`config/` 下对应的板卡配置以及下表中的屏幕配置：

| 屏幕 | 分辨率 | 最后追加的配置 |
| --- | --- | --- |
| 7-DSI-TOUCH-A | 720 × 1280 | `config/lcd-7-a.defaults` |
| 8-DSI-TOUCH-A | 800 × 1280 | `config/lcd-8-a.defaults` |
| 10.1-DSI-TOUCH-A | 800 × 1280 | `sdkconfig.defaults` 已选择 |

1. 为每种「板卡 + 屏幕」组合指定独立的 `-B` 构建目录和 `SDKCONFIG` 文件，执行 `idf.py build`。
2. 构建成功后，执行以下命令生成合并 BIN（带音频的联网版本会将小智唤醒模型合并进同一 BIN）：

   ```bash
   idf.py -B <构建目录> merge-bin -f raw -t 0x0 -o <绝对输出路径>.bin
   ```
