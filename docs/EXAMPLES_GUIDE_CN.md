# 示例指南

[English Version](./EXAMPLES_GUIDE.md)

本仓库服务两类用户：

- 需要清晰完成第一次成功运行的初学者。
- 希望复制聚焦外设示例到实际项目中的高级用户。

使用本指南选择合适的示例。

烧录前，请确认 ESP32-P4 芯片版本系列与构建配置匹配。在 ESP-IDF 中，量产 v3.x 芯片和更早的 pre-v3 工程样片是互斥的构建目标。共享的 `config/` overlay 和可复制命令请参见 [ESP32-P4 版本配置](ESP32P4_REVISION_CONFIG_CN.md)。

## 学习路径

| 等级 | 目标 | 示例 |
| --- | --- | --- |
| 0 | 验证开发板、烧录、PSRAM、heap 和 monitor | `examples/esp-idf/00_board_check` |
| 1 | 学习标准 ESP-IDF 应用循环 | `examples/esp-idf/02_HelloWorld` |
| 2 | 学习持久化设置和任务 | `03_nvs_counter`, `04_freertos_tasks` |
| 3 | 尝试简单开发板 I/O | `05_gpio_io`, `06_gpio_interrupt`, `07_uart_loopback`, `08_i2c_tools` |
| 4 | bring up 一个开发板外设 | Wi-Fi、Ethernet、SD card、display 或 audio 示例 |
| 5 | 组合 display、camera、network 和 UI | Video 和 ESP-Brookesia 示例 |
| Arduino | 从显示输出开始 | `examples/arduino/examples/HelloWorld` |

## ESP-IDF 示例

| 示例 | 难度 | 外部硬件 | 适合用途 |
| --- | --- | --- | --- |
| `00_board_check` | 初学者 | 无 | 第一次启动、烧录、串口监视器、PSRAM 检查 |
| `01_HowToCreateProject` | 初学者 | 无 | 理解最小工程结构 |
| `02_HelloWorld` | 初学者 | 无 | 经典 ESP-IDF 冒烟测试 |
| `03_nvs_counter` | 初学者 | 无 | 保存小型持久化数值 |
| `04_freertos_tasks` | 初学者到中级用户 | 无 | 任务、队列和周期性工作 |
| `05_gpio_io` | 初学者 | 可选跳线或 LED | GPIO 输入/输出基础 |
| `06_gpio_interrupt` | 初学者到中级用户 | 推荐按钮或跳线 | 中断和消抖 |
| `07_uart_loopback` | 初学者到中级用户 | 推荐跳线 | UART 发送/接收工作流 |
| `08_i2c_tools` | 初学者到中级用户 | 可选 I2C 设备 | 扫描和调试 I2C 总线 |
| `09_sdmmc` | 中级用户 | SD card | 挂载和测试 SD card |
| `10_wifistation` | 中级用户 | 支持 Wi-Fi 的开发板或 Wi-Fi 支持路径 | 连接到接入点 |
| `11_ethernetbasic` | 中级用户 | Ethernet PHY/module | Ethernet link 和 DHCP bring-up |
| `12_I2SCodec` | 中级用户 | Audio codec | 音乐播放或 echo mode |
| `13_Displaycolorbar` | 中级用户 | 支持的 LCD | Display panel bring-up |
| `14_lvgl_demo_v9` | 高级用户 | 支持的 LCD，通常需要 PSRAM | LVGL v9 UI demo |
| `15_eth2ap` | 高级用户 | Ethernet 和 Wi-Fi 路径 | 网络桥接工作流 |
| `16_video_lcd_display` | 高级用户 | Camera 和 LCD | Camera 到 display pipeline |
| `17_simple_video_server` | 高级用户 | Camera 和 network | 基于浏览器的 camera streaming |
| `18_esp_brookesia_phone` | 高级用户 | LCD/touch 和受支持开发板 profile | ESP-Brookesia UI 应用 |
| `19_system_monitor` | 高级用户 | 无 | 串口诊断、运行时采样和持久化 monitor 设置 |

## Arduino 示例

| 示例 | 难度 | 外部硬件 | 说明 |
| --- | --- | --- | --- |
| `HelloWorld` | 初学者 | 支持的 DSI display | 第一个 Arduino display 测试 |
| `AsciiTable` | 初学者 | 支持的 DSI display | 文本渲染网格 |
| `Drawing_board` | 中级用户 | DSI display 和 touch | 触摸绘图 demo |
| `GFX_ESPWiFiAnalyzer` | 中级用户 | DSI display 和 Wi-Fi | Wi-Fi 扫描可视化 |
| `LVGLV9_Arduino` | 高级用户 | DSI display、touch、PSRAM | Arduino 上的 LVGL v9 demo |
| `Camera_Preview` | 中级用户 | OV5647、DSI display、PSRAM | 使用共用 Platform I2C bus 的 MIPI-CSI 预览 |
| `Camera_ISP_Tuning` | 高级用户 | OV5647、DSI display、PSRAM | 通过非阻塞串口命令调校 camera ISP |
| `SD_Card` | 中级用户 | 已插入的 SD 卡 | 四位 SDMMC 写入/回读检查 |
| `Audio_Playback` | 中级用户 | ES8311 和板载 speaker | I2S 音调播放；不是 LCD-5 的 ES7210 麦克风路径 |

## 值得补齐的空白

当前示例已经覆盖许多主要外设，但更多小示例会让仓库更友好。适合新增的方向包括：

- UART 外部模块通信。
- ADC：单次读取和周期性采样。
- SPI：简单传感器或与显示无关的总线扫描模式。
- Wi-Fi SoftAP：最小接入点示例。
- HTTP client：调用公共 API 或本地服务器。
- 文件系统基础：无需 SD card 硬件的 SPIFFS 或 LittleFS。
- 电源管理：light sleep 和唤醒源。
- 从完整 LVGL demo 中拆出显示基础：backlight、color fill、rotation。
- 从 drawing board 中拆出触摸基础：原始触摸坐标日志。

添加新示例时，请让它聚焦一个概念，并包含 README，说明硬件要求、构建命令、预期输出和已知限制。
