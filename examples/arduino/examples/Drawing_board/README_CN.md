# Drawing Board

[English](README.md)

使用 DSI 显示屏和 GT911 触摸输入的 Arduino 绘图示例。

## 难度

中级。

## 所需硬件

- ESP32-P4 开发板。
- 受支持的 DSI 触摸显示屏。
- 在 Arduino 开发板设置中启用 PSRAM。

## 运行方法

1. 阅读 [../../README_CN.md](../../README_CN.md)。
2. 打开 `Drawing_board.ino`。
3. 选择正确的开发板、端口和 PSRAM 设置。
4. 如有需要，设置 `CURRENT_SCREEN`。
5. 上传 sketch。

## 预期行为

显示屏显示绘图区域，触摸输入会在屏幕上绘制内容。

## 故障排查

- 如果显示正常但触摸无响应，请检查 GT911 I2C 路径。
- 确认所选显示配置与实际屏幕相匹配。
