# ESP32-P4 Platform 示例

[English Version](./README.md)

本仓库提供 Waveshare ESP32-P4 系列开发板的开源示例和板级 bring-up 参考。

本仓库收集了 ESP-IDF 和 Arduino 示例，覆盖 ESP32-P4 平台常见能力，包括
GPIO、I2C、Wi-Fi、Ethernet、SD card、audio codec、display、LVGL、
camera、video streaming 和 ESP-Brookesia UI 工作流。

## 支持的开发板

| 开发板 | 产品 |
| --- | --- |
| ESP32-P4-NANO | <a href="https://www.waveshare.net/shop/ESP32-P4-NANO.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-nano-1.jpg" width="150"></a> |
| ESP32-P4-Module-DEV-KIT | <a href="https://www.waveshare.net/shop/ESP32-P4-Module-DEV-KIT.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-module-dev-kit-0.jpg" width="150"></a> |
| ESP32-P4-WIFI6-DEV-KIT | <a href="https://www.waveshare.net/shop/ESP32-P4-WIFI6-DEV-KIT.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-wifi6-dev-kit-1.jpg" width="150"></a> |
| ESP32-P4-WIFI6 | <a href="https://www.waveshare.net/shop/ESP32-P4-WIFI6.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-wifi6-1.jpg" width="150"></a> |
| ESP32-P4-ETH | <a href="https://www.waveshare.net/shop/ESP32-P4-ETH.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-eth-1.jpg" width="150"></a> |
| ESP32-P4-Pico | <a href="https://www.waveshare.net/shop/ESP32-P4-Pico.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-pico-1.jpg" width="150"></a> |
| ESP32-P4-WIFI6-POE-ETH | <a href="https://www.waveshare.net/shop/ESP32-P4-WIFI6-POE-ETH.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-wifi6-poe-eth-1_2.jpg" width="150"></a> |
| ESP32-P4-Core-DEV-KIT | <a href="https://www.waveshare.net/shop/ESP32-P4-Core-DEV-KIT.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-core-dev-kit-1.jpg" width="150"></a> |

## 仓库结构

```text
.
├── examples/
│   ├── esp-idf/      ESP-IDF 示例，每个目录一个工程
│   └── arduino/      Arduino sketches 和随附示例库
├── config/           共享 ESP-IDF 配置 overlay
├── docs/             仓库结构和维护者说明
├── .github/          Issue 和 Pull Request 模板
├── CONTRIBUTING.md   贡献流程
├── SUPPORT.md        支持渠道
├── SECURITY.md       安全漏洞报告策略
└── LICENSE.txt       Apache-2.0 许可证
```

如果是第一次使用本仓库，请从 [快速开始](docs/GETTING_STARTED_CN.md) 开始。
关于新增示例和文档应放置的位置，请参见
[项目结构](docs/PROJECT_STRUCTURE_CN.md)。

## 文档

| 文档 | 读者 | 用途 |
| --- | --- | --- |
| [快速开始](docs/GETTING_STARTED_CN.md) | 初学者 | 第一次构建、烧录和 monitor 工作流 |
| [示例指南](docs/EXAMPLES_GUIDE_CN.md) | 所有用户 | 按难度和外设选择示例 |
| [ESP32-P4 版本配置](docs/ESP32P4_REVISION_CONFIG_CN.md) | 所有用户 | 选择 v3.x 量产或 pre-v3 样片构建 profile |
| [故障排查](docs/TROUBLESHOOTING_CN.md) | 所有用户 | 构建、烧录、运行时和外设检查 |
| [持续集成](docs/CI_CN.md) | 贡献者 | ESP-IDF 示例的 GitHub Actions 覆盖 |
| [示例路线图](docs/EXAMPLE_ROADMAP_CN.md) | 贡献者 | 可改进覆盖范围的建议示例 |
| [项目结构](docs/PROJECT_STRUCTURE_CN.md) | 贡献者 | 仓库布局和示例要求 |

## 快速开始

### ESP-IDF 示例

1. 安装 ESP-IDF。本仓库中大多数示例基于 ESP-IDF release/v5.4 或更高版本
   准备。部分高级 UI 示例会在自己的 README 中记录 release/v5.3 兼容性。
2. 克隆本仓库。
3. 如果你是第一次使用该开发板或本仓库，请先运行
   [00_board_check](examples/esp-idf/00_board_check/)。
4. 设置目标芯片并构建：

```bash
cd examples/esp-idf/00_board_check
idf.py set-target esp32p4
idf.py build
```

5. 如果你使用早于 rev v3.0 的 ESP32-P4 早期工程样片，或者需要将量产固件
   固定到 rev v3.0 或 v3.1，请在烧录前阅读
   [ESP32-P4 版本配置](docs/ESP32P4_REVISION_CONFIG_CN.md)。
6. 烧录并打开 monitor，将 `PORT` 替换为你的串口：

```bash
idf.py -p PORT flash monitor
```

构建需要板级专属设置、网络凭据、显示选项、摄像头传感器或 PHY 配置的示例
前，请先运行 `idf.py menuconfig`。

### Arduino 示例

Arduino 示例位于 [examples/arduino](examples/arduino)。推荐从
[examples/arduino/README.md](examples/arduino/README.md) 开始，了解推荐的
Arduino-ESP32 core、随附 LVGL 版本、Arduino_GFX 版本以及 I2C driver 兼容性
说明。

## 示例索引

| 编号 | 示例 | 范围 |
| --- | --- | --- |
| 00 | [board_check](examples/esp-idf/00_board_check/) | 首次运行的开发板和工具链检查 |
| 01 | [HowToCreateProject](examples/esp-idf/01_HowToCreateProject/) | 最小 ESP-IDF 工程结构 |
| 02 | [HelloWorld](examples/esp-idf/02_HelloWorld/) | 基础 ESP-IDF 应用 |
| 03 | [nvs_counter](examples/esp-idf/03_nvs_counter/) | 使用 NVS 保存持久化设置 |
| 04 | [freertos_tasks](examples/esp-idf/04_freertos_tasks/) | 任务和队列 |
| 05 | [gpio_io](examples/esp-idf/05_gpio_io/) | GPIO 输入/输出 |
| 06 | [gpio_interrupt](examples/esp-idf/06_gpio_interrupt/) | GPIO 中断和消抖 |
| 07 | [uart_loopback](examples/esp-idf/07_uart_loopback/) | UART 发送/接收回环 |
| 08 | [i2c_tools](examples/esp-idf/08_i2c_tools/) | I2C 扫描和命令行工具 |
| 09 | [sdmmc](examples/esp-idf/09_sdmmc/) | SD card 和 SDMMC |
| 10 | [wifistation](examples/esp-idf/10_wifistation/) | Wi-Fi Station 连接 |
| 11 | [ethernetbasic](examples/esp-idf/11_ethernetbasic/) | Ethernet bring-up |
| 12 | [I2SCodec](examples/esp-idf/12_I2SCodec/) | I2S audio codec |
| 13 | [Displaycolorbar](examples/esp-idf/13_Displaycolorbar/) | LCD display color bar |
| 14 | [lvgl_demo_v9](examples/esp-idf/14_lvgl_demo_v9/) | LVGL v9 display demo |
| 15 | [eth2ap](examples/esp-idf/15_eth2ap/) | Ethernet-to-AP 网络 |
| 16 | [video_lcd_display](examples/esp-idf/16_video_lcd_display/) | 摄像头视频输出到 LCD |
| 17 | [simple_video_server](examples/esp-idf/17_simple_video_server/) | HTTP 摄像头流媒体 |
| 18 | [esp_brookesia_phone](examples/esp-idf/18_esp_brookesia_phone/) | ESP-Brookesia Phone UI |
| 19 | [system_monitor](examples/esp-idf/19_system_monitor/) | 串口诊断和运行时监控 |

如需更完整的 ESP-IDF 和 Arduino 示例地图，请参见
[examples/README.md](examples/README.md) 和
[示例指南](docs/EXAMPLES_GUIDE_CN.md)。

## ESP-IDF 兼容性矩阵

图例：`✅` 表示该示例预期可在该开发板上运行。`❌` 表示示例所需外设不可用
或该示例未支持该开发板。

| 编号 | 示例 | ESP32-P4-NANO | ESP32-P4-Module-DEV-KIT | ESP32-P4-WIFI6-DEV-KIT | ESP32-P4-WIFI6 | ESP32-P4-ETH | ESP32-P4-Pico | ESP32-P4-WIFI6-POE-ETH | ESP32-P4-Core-DEV-KIT |
| --- | --- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| - | 产品页 | <a href="https://www.waveshare.net/shop/ESP32-P4-NANO.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-nano-1.jpg" width="120"></a> | <a href="https://www.waveshare.net/shop/ESP32-P4-Module-DEV-KIT.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-module-dev-kit-0.jpg" width="120"></a> | <a href="https://www.waveshare.net/shop/ESP32-P4-WIFI6-DEV-KIT.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-wifi6-dev-kit-1.jpg" width="120"></a> | <a href="https://www.waveshare.net/shop/ESP32-P4-WIFI6.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-wifi6-1.jpg" width="120"></a> | <a href="https://www.waveshare.net/shop/ESP32-P4-ETH.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-eth-1.jpg" width="120"></a> | <a href="https://www.waveshare.net/shop/ESP32-P4-Pico.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-pico-1.jpg" width="120"></a> | <a href="https://www.waveshare.net/shop/ESP32-P4-WIFI6-POE-ETH.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-wifi6-poe-eth-1_2.jpg" width="120"></a> | <a href="https://www.waveshare.net/shop/ESP32-P4-Core-DEV-KIT.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-core-dev-kit-1.jpg" width="120"></a> |
| 00 | [board_check](examples/esp-idf/00_board_check/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 01 | [HowToCreateProject](examples/esp-idf/01_HowToCreateProject/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 02 | [HelloWorld](examples/esp-idf/02_HelloWorld/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 03 | [nvs_counter](examples/esp-idf/03_nvs_counter/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 04 | [freertos_tasks](examples/esp-idf/04_freertos_tasks/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 05 | [gpio_io](examples/esp-idf/05_gpio_io/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 06 | [gpio_interrupt](examples/esp-idf/06_gpio_interrupt/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 07 | [uart_loopback](examples/esp-idf/07_uart_loopback/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 08 | [i2c_tools](examples/esp-idf/08_i2c_tools/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 09 | [sdmmc](examples/esp-idf/09_sdmmc/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ |
| 10 | [wifistation](examples/esp-idf/10_wifistation/) | ✅ | ✅ | ✅ | ✅ | ❌ | ❌ | ✅ | ❌ |
| 11 | [ethernetbasic](examples/esp-idf/11_ethernetbasic/) | ✅ | ✅ | ✅ | ❌ | ✅ | ❌ | ✅ | ❌ |
| 12 | [I2SCodec](examples/esp-idf/12_I2SCodec/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ |
| 13 | [Displaycolorbar](examples/esp-idf/13_Displaycolorbar/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 14 | [lvgl_demo_v9](examples/esp-idf/14_lvgl_demo_v9/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 15 | [eth2ap](examples/esp-idf/15_eth2ap/) | ✅ | ✅ | ✅ | ❌ | ❌ | ❌ | ✅ | ❌ |
| 16 | [video_lcd_display](examples/esp-idf/16_video_lcd_display/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 17 | [simple_video_server](examples/esp-idf/17_simple_video_server/) | ✅ | ✅ | ✅ | ✅ | ✅, Ethernet | ❌ | ✅ | ❌ |
| 18 | [esp_brookesia_phone](examples/esp-idf/18_esp_brookesia_phone/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 19 | [system_monitor](examples/esp-idf/19_system_monitor/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |

## 贡献

欢迎贡献。提交 Pull Request 前，请阅读 [CONTRIBUTING.md](CONTRIBUTING.md)。
有价值的贡献包括：

- 修复受支持开发板上的构建问题。
- 在现有示例 README 中添加板级专属说明。
- 改进 setup 说明或 troubleshooting 说明。
- 添加目前尚未覆盖的聚焦外设示例。

报告问题时，请包含开发板名称、ESP-IDF 或 Arduino-ESP32 版本、串口日志和
具体示例路径。

支持渠道说明请参见 [SUPPORT.md](SUPPORT.md)。

## 第三方代码

部分示例随附第三方组件或库以方便使用，包括 LVGL、Arduino_GFX、
ESP-Brookesia 相关组件和 Espressif managed components。高层级清单请参见
[THIRD_PARTY.md](THIRD_PARTY.md)，并检查每个随附组件自己的许可证文件。

## 安全

请不要在公开 issue 中报告安全漏洞。请按照 [SECURITY.md](SECURITY.md) 进行
负责任披露。

## 许可证

本仓库基于 Apache License 2.0 授权。详情请参见 [LICENSE.txt](LICENSE.txt)。
