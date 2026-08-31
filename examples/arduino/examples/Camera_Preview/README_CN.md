# Camera Preview

[English Version](README.md)

在 Platform 显示屏上预览 OV5647 摄像头。

## 要求

- Arduino-ESP32 `3.3.11`、`ESP32P4 Dev Module`、已启用 PSRAM，以及默认
  `ChipVariant=postv3`（v3.00+、400 MHz）。
- 受支持的 OV5647 摄像头和 Platform 显示屏。
- 共用的 Platform I2C bus；不要另建 camera-control bus。

## 构建

在 Arduino IDE 中打开 `Camera_Preview.ino`，或使用仓库文档中的 Arduino CLI FQBN
和 board options 编译该 sketch。

## 预期结果

sketch 初始化受支持的摄像头并显示预览。编译只检查软件配置，不能证明真实硬件上的
camera capture、display 或 I2C 行为。

## HIL 清单

关闭所有串口监视器后完全断电重启，确认预览启动；随后以 `115200` 打开 UART0，确认
应用持续运行。记录开发板版本、摄像头模块和结果。

## 故障排查

确认 OV5647 模块、PSRAM、显示选择和共用 I2C 路径。包括 v1.3 在内的 pre-v3 silicon
请显式使用 `ChipVariant=prev3`/360 MHz，且不要复用 post-v3 构建输出。
