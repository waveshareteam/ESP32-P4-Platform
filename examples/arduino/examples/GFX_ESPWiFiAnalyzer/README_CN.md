# GFX ESP Wi-Fi Analyzer

[English](README.md)

使用 Arduino_GFX 的 Arduino Wi-Fi 扫描可视化示例。

## 难度

中级。

## 所需硬件

- 具备 Wi-Fi 支持的 ESP32-P4 开发板。
- 受支持的 DSI 显示屏。
- 在 Arduino 开发板设置中启用 PSRAM。

## 运行方法

1. 阅读 [../../README_CN.md](../../README_CN.md)。
2. 打开 `GFX_ESPWiFiAnalyzer.ino`。
3. 选择正确的开发板、端口和 PSRAM 设置。
4. 如有需要，设置 `CURRENT_SCREEN`。
5. 上传 sketch。

## 预期行为

显示屏显示附近的 Wi-Fi 网络及其信号强度。

## 故障排查

- 确认你的开发板支持 Wi-Fi。
- 先使用更简单的 `HelloWorld` sketch 确认显示屏能够初始化。
