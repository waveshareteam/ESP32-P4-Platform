# Arduino Hello World

[English](README.md)

用于受支持 ESP32-P4 DSI 显示配置的最小 Arduino 显示测试。

## 难度

初级。

## 所需硬件

- ESP32-P4 开发板。
- 受支持的 DSI 显示屏。
- 在 Arduino 开发板设置中启用 PSRAM。

## 运行方法

1. 阅读 [../../README_CN.md](../../README_CN.md)，了解 Arduino core 和库说明。
2. 在 Arduino IDE 中打开 `HelloWorld.ino`。
3. 选择正确的 ESP32-P4 开发板和端口。
4. 启用 PSRAM。
5. 如果显示屏并非默认配置，请设置 `CURRENT_SCREEN`。
6. 上传 sketch。

## 预期行为

屏幕初始化、打开背光并显示 `Hello World!`。
