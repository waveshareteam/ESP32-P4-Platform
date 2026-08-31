# 示例

[English Version](README.md)

本目录包含面向 Waveshare ESP32-P4 平台开发板的 ESP-IDF 工程和 Arduino
sketch。

## ESP-IDF

[esp-idf](esp-idf/) 下的每个目录都是独立 ESP-IDF 工程。除非示例 README
另有说明，请在所选示例目录中运行 ESP-IDF 命令。

```bash
cd examples/esp-idf/00_board_check
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

ESP32-P4 量产 v3.x 芯片和更早的 pre-v3 芯片需要不同构建 profile。默认是
`rev3_x`/`MIN_300`；包括 v1.3 在内的 pre-v3 silicon 显式选择 legacy
`rev1_3`/`MIN_100`。示例通过 [../config](../config/) 中的共享 overlay 选择芯片版本；
生成的 `sdkconfig` 和二进制不可互换。跨芯片版本系列烧录前，请阅读
[ESP32-P4 版本配置](../docs/ESP32P4_REVISION_CONFIG_CN.md)。

托管依赖策略和示例本地组件分类见
[托管组件](../docs/COMPONENTS_CN.md)。P4 通过 C6 协处理器使用 Wi-Fi 时，
还应阅读 [P4 + C6 Hosted Wi-Fi](../docs/P4_C6_HOSTED_WIFI_CN.md)。

| 目录 | 用途 | 说明 |
| --- | --- | --- |
| [00_board_check](esp-idf/00_board_check/) | 首次运行的开发板和工具链检查 | 不需要外接硬件 |
| [01_HowToCreateProject](esp-idf/01_HowToCreateProject/) | 最小工程模板 | 适合作为新示例起点 |
| [02_HelloWorld](esp-idf/02_HelloWorld/) | 基础应用和日志 | 快速检查环境 |
| [03_nvs_counter](esp-idf/03_nvs_counter/) | 使用 NVS 保存持久化设置 | 不需要外接硬件 |
| [04_freertos_tasks](esp-idf/04_freertos_tasks/) | FreeRTOS 任务和队列 | 不需要外接硬件 |
| [05_gpio_io](esp-idf/05_gpio_io/) | GPIO 输入/输出 | 可选跳线或 LED |
| [06_gpio_interrupt](esp-idf/06_gpio_interrupt/) | GPIO 中断和消抖 | 建议使用按键或跳线 |
| [07_uart_loopback](esp-idf/07_uart_loopback/) | UART 回环 | 建议使用跳线 |
| [08_i2c_tools](esp-idf/08_i2c_tools/) | I2C 工具 | 包含命令处理程序 |
| [09_sdmmc](esp-idf/09_sdmmc/) | SD 卡 | 需要 SD 卡接线或板载卡槽支持 |
| [10_wifistation](esp-idf/10_wifistation/) | Wi-Fi Station | 需要带 Wi-Fi 的板卡或已配置 remote Wi-Fi |
| [11_ethernetbasic](esp-idf/11_ethernetbasic/) | Ethernet | 检查 menuconfig 中的 PHY 设置 |
| [12_I2SCodec](esp-idf/12_I2SCodec/) | I2S audio codec | 包含音频示例数据 |
| [13_Displaycolorbar](esp-idf/13_Displaycolorbar/) | 显示色条 | 适合 LCD bring-up |
| [14_lvgl_demo_v9](esp-idf/14_lvgl_demo_v9/) | LVGL v9 | 使用托管 LVGL/BSP 组件 |
| [15_eth2ap](esp-idf/15_eth2ap/) | Ethernet 转 AP | 需要两条网络路径 |
| [16_video_lcd_display](esp-idf/16_video_lcd_display/) | 摄像头视频输出到显示屏 | 请先阅读示例 README |
| [17_simple_video_server](esp-idf/17_simple_video_server/) | HTTP 视频服务器 | 包含前端资源 |
| [18_esp_brookesia_phone](esp-idf/18_esp_brookesia_phone/) | ESP-Brookesia UI | 含中英文 README |
| [19_system_monitor](esp-idf/19_system_monitor/) | 串口诊断和运行时监控 | 不需要外接硬件 |

## Arduino

Arduino 示例位于 [arduino/examples](arduino/examples/)。
[Arduino 中文指南](arduino/README_CN.md)记录推荐的 Arduino-ESP32 core、随附库
和开发板选项；也可阅读 [English guide](arduino/README.md)。

| 目录 | 用途 |
| --- | --- |
| [AsciiTable](arduino/examples/AsciiTable/) | 串口输出示例 |
| [Drawing_board](arduino/examples/Drawing_board/) | 显示/触摸绘图示例 |
| [GFX_ESPWiFiAnalyzer](arduino/examples/GFX_ESPWiFiAnalyzer/) | Wi-Fi analyzer UI |
| [HelloWorld](arduino/examples/HelloWorld/) | 最小 Arduino sketch |
| [LVGLV9_Arduino](arduino/examples/LVGLV9_Arduino/) | LVGL v9 Arduino 示例 |
| [Camera_Preview](arduino/examples/Camera_Preview/) | OV5647 摄像头预览；需要 PSRAM 和 Platform I2C bus |
| [Camera_ISP_Tuning](arduino/examples/Camera_ISP_Tuning/) | OV5647 ISP 调校；需要 PSRAM 和 Platform I2C bus |
| [SD_Card](arduino/examples/SD_Card/) | SD 卡访问；需要插入卡 |
| [Audio_Playback](arduino/examples/Audio_Playback/) | 使用板载 speaker 的 ES8311 播放 |

Arduino 默认使用 `ChipVariant=postv3`（v3.00+、400 MHz）；只有 legacy pre-v3
开发板才显式使用 `prev3`/360 MHz。Camera sketch 需要受支持的 OV5647 和 PSRAM，且
复用 Platform I2C bus。Platform 音频 surface 是 ES8311 输出，因此 LCD-5 的 ES7210
`Mic_Record` 不属于本产品示例集。编译不等于硬件验证。

[arduino/libraries](arduino/libraries/) 包含这些 sketch 使用的随附库。单独再发布
某个库前，请检查该库自己的 metadata 和许可证。

## 添加新示例

新示例应保持小而聚焦：

- 独立 ESP-IDF 示例使用带编号的目录。
- 按需包含 `README.md`、`README_CN.md`、`CMakeLists.txt`、`main/` 和
  `sdkconfig.defaults`。
- 记录所需硬件、menuconfig 选项和预期串口输出。
- 除非经过有意维护，否则不要提交生成的 `build/`、`sdkconfig`、
  `managed_components/` 和依赖锁文件。

学习路径见[示例指南](../docs/EXAMPLES_GUIDE_CN.md)，建议补充的示例见
[示例路线图](../docs/EXAMPLE_ROADMAP_CN.md)。
