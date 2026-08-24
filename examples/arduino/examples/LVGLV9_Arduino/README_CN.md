# LVGL v9 Arduino

[English](README.md)

面向受支持 ESP32-P4 显示和触摸硬件的 Arduino LVGL v9 演示。

## 难度

高级。

## 所需硬件

- ESP32-P4 开发板。
- 受支持的 DSI 触摸显示屏。
- 在 Arduino 开发板设置中启用 PSRAM。

## 运行方法

1. 阅读 [../../README_CN.md](../../README_CN.md)。
2. 打开 `LVGLV9_Arduino.ino`。
3. 选择正确的开发板、端口和 PSRAM 设置。
4. 如有需要，设置 `CURRENT_SCREEN`。
5. 上传 sketch。

## 预期行为

显示屏初始化并运行支持触摸输入的 LVGL 演示。

## 故障排查

- 先运行 Arduino `HelloWorld` 以验证显示路径。
- 先运行 `Drawing_board` 以验证触摸输入。
- 如果开发板重启，请检查 LVGL 内存和绘制缓冲区设置。
