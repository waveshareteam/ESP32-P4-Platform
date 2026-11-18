# 出厂固件

[English](README.md) | [简体中文](README_ZH.md)

[manifest.json](manifest.json) 收录完成实机验证的镜像。下载前核对板型、芯片修订版、Flash 容量和屏幕型号。

| 板卡 | 网络 | Flash | 出厂配置 |
| --- | --- | ---: | --- |
| [ESP32-P4-NANO](https://docs.waveshare.com/ESP32-P4-NANO) | Wi-Fi + ETH | 16 MB | `wifi-eth-16.defaults` |
| [ESP32-P4-Module-DEV-KIT](https://docs.waveshare.com/ESP32-P4-Module-DEV-KIT) | Wi-Fi + ETH | 16 MB | `wifi-eth-16.defaults` |
| [ESP32-P4-WIFI6-DEV-KIT](https://docs.waveshare.com/ESP32-P4-WIFI6-DEV-KIT) | Wi-Fi + ETH | 16 MB | `wifi-eth-16.defaults` |
| [ESP32-P4-WIFI6-POE-ETH](https://docs.waveshare.com/ESP32-P4-WIFI6-POE-ETH) | Wi-Fi + ETH | 32 MB | `wifi-eth-32.defaults` |
| [ESP32-P4-WIFI6](https://docs.waveshare.com/ESP32-P4-WIFI6) | Wi-Fi | 32 MB | `wifi-32.defaults` |
| [ESP32-P4-ETH](https://docs.waveshare.com/ESP32-P4-ETH) | ETH | 32 MB | `eth-32.defaults` |
| [ESP32-P4-Pico](https://docs.waveshare.com/ESP32-P4-Pico) | 无板载网络 | 32 MB | `offline-32.defaults` |
| [ESP32-P4-Core-DEV-KIT](https://docs.waveshare.com/ESP32-P4-Core-DEV-KIT) | 无板载网络 | 32 MB | `offline-32.defaults` + `core-dev-kit.defaults` |

每种配置按屏幕分别构建。`7-DSI-TOUCH-A` 为 720×1280，`8-DSI-TOUCH-A` 和 `10.1-DSI-TOUCH-A` 都是 800×1280，但屏幕初始化配置不同。以上配置面向 rev3.x 芯片，同名板卡可能有不同芯片修订版。

合并 BIN 从 **`0x0`** 烧录。镜像会覆盖 Flash 的原有应用和设置，烧录前请按需备份。板卡目录中的说明列出了对应镜像及命令。[出厂工程构建说明](../examples/factory/README_ZH.md)

## 从源码构建

在 `examples/factory/` 目录下，按顺序使用 `sdkconfig.defaults`、`sdkconfig.rev3_x.defaults` 和上表的板卡配置，再按下表选择屏幕。

| 屏幕 | 分辨率 | 最后追加的配置 |
| --- | --- | --- |
| 7-DSI-TOUCH-A | 720×1280 | `config/lcd-7-a.defaults` |
| 8-DSI-TOUCH-A | 800×1280 | `config/lcd-8-a.defaults` |
| 10.1-DSI-TOUCH-A | 800×1280 | `sdkconfig.defaults` 已选择 |

每种组合使用独立的 `-B` 构建目录和 `SDKCONFIG` 文件。`idf.py build` 成功后执行 `idf.py -B <构建目录> merge-bin -f raw -t 0x0 -o <绝对输出路径>.bin`。带音频的联网版本会将小智唤醒模型合并进同一镜像。

镜像按 `wifi-eth`、`wifi-only`、`eth-only` 和 `offline` 分类。`manifest.json` 使用[多产品清单格式](https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-X/blob/main/firmware/manifest.json)，其中的路径相对于仓库根目录。
