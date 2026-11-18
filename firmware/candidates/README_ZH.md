# 待实测候选固件

[English](README.md) | [简体中文](README_ZH.md)

这 18 个合并 BIN 已通过本地构建和镜像校验，供硬件验证使用。正式出厂镜像通过[实测固件清单](../manifest.json)登记。

请按板型、Flash 容量和实际连接的屏幕选用。候选镜像面向 ESP32-P4 芯片 v3.x，分别提供 7-DSI-TOUCH-A、8-DSI-TOUCH-A 和 10.1-DSI-TOUCH-A 构建。各档位说明包含 BIN 链接和从 `0x0` 烧录的命令。

| 网络 | 板卡 | 候选档位 |
| --- | --- | --- |
| Wi-Fi + ETH | ESP32-P4-NANO、ESP32-P4-Module-DEV-KIT、ESP32-P4-WIFI6-DEV-KIT | [16 MB](wifi-eth/wifi-eth-16/README_ZH.md) |
| Wi-Fi + ETH | ESP32-P4-WIFI6-POE-ETH | [32 MB](wifi-eth/wifi-eth-32/README_ZH.md) |
| Wi-Fi | ESP32-P4-WIFI6 | [32 MB](wifi-only/wifi-32/README_ZH.md) |
| ETH | ESP32-P4-ETH | [32 MB](eth-only/eth-32/README_ZH.md) |
| 离线 | ESP32-P4-Pico | [32 MB](offline/offline-32/README_ZH.md) |
| 离线 | ESP32-P4-Core-DEV-KIT | [32 MB、无板载音频和 microSD](offline/core-dev-kit/README_ZH.md) |

[候选 manifest](manifest.json) 使用相对于仓库根目录的路径。小智和 WakeNet 资源的许可证见[第三方说明](../../docs/THIRD_PARTY_ZH.md)。
