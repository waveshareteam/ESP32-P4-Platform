# 示例路线图

[English Version](./EXAMPLE_ROADMAP.md)

这份路线图用于跟踪示例覆盖情况，目标是让本仓库同时更好地服务初学者和高级用户。

## 当前优势

- ESP-IDF 工程基础。
- I2C 命令行工具。
- NVS 持久化设置。
- FreeRTOS 任务和队列基础。
- UART 回环。
- GPIO 输入/输出基础。
- GPIO 中断和消抖。
- Wi-Fi station 和 Ethernet 示例。
- SDMMC 卡示例。
- I2S codec 示例。
- Display、LVGL、camera、video server 和 ESP-Brookesia 示例。
- Arduino display、touch、camera、SD 卡和 ES8311 播放示例；在需要时随附依赖库。

## 高优先级新增项

| 优先级 | 示例 | 受众 | 重要性 |
| --- | --- | --- | --- |
| 高 | Display backlight and color fill | 初学者 | 比完整 LVGL 更适合显示 bring-up |
| 高 | Touch coordinate logger | 初学者到中级用户 | 将触摸 bring-up 与绘图/UI 代码分离 |
| 高 | UART external module template | 中级用户 | 在 loopback 基础上扩展到真实设备集成 |

## 中优先级新增项

| 优先级 | 示例 | 受众 | 重要性 |
| --- | --- | --- | --- |
| 中 | ADC periodic sampling | 初学者 | 教授模拟输入和定时器 |
| 中 | SPI device template | 中级用户 | 帮助用户添加传感器和模块 |
| 中 | Wi-Fi SoftAP | 中级用户 | 在没有路由器时很有用 |
| 中 | HTTP client | 中级用户 | 常见 IoT 工作流 |
| 中 | SPIFFS or LittleFS basics | 中级用户 | 无需 SD 硬件的文件系统工作流 |
| 中 | Power management | 高级用户 | 产品中的睡眠和唤醒模式 |

## 文档工作

- 为引脚映射和支持的扩展模块添加开发板专属说明。
- 为简单示例添加预期串口输出片段。
- 为 display、touch、camera 和 network 示例添加故障排查章节。

## 新示例质量标准

每个新示例都应包含：

- 聚焦明确的目标。
- 所需硬件。
- 支持的开发板或已知开发板假设。
- 构建和烧录命令。
- `menuconfig` 选项（如有）。
- 预期串口输出或可见行为。
- 已知限制。
