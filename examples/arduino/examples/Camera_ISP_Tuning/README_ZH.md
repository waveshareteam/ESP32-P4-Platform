# Camera ISP Tuning

[English Version](README.md)

在 Platform 显示屏上运行 OV5647 摄像头 ISP 调校路径。

## 要求

- Arduino-ESP32 `3.3.11`、`ESP32P4 Dev Module`、已启用 PSRAM，以及默认
  `ChipVariant=postv3`（v3.00+、400 MHz）。
- 受支持的 OV5647 摄像头和 Platform 显示屏。
- 共用的 Platform I2C bus；不要使用单独的 camera-control bus。

## 构建

在 Arduino IDE 中打开 `Camera_ISP_Tuning.ino`，或使用仓库文档中的 FQBN 和
board options 编译。

## 预期结果

sketch 启动 camera/display 路径并提供 ISP 调校行为。编译不能验证 sensor tuning、
图像质量或 I2C 电气行为。

## HIL 清单

关闭 monitor 后冷启动，确认预期的调校 UI 或预览；随后以 `115200` 打开 UART0，应用
不得复位或卡死。

## 故障排查

检查 OV5647、PSRAM、显示选择和共用 I2C bus。pre-v3/v1.3 开发板需要单独显式使用
`ChipVariant=prev3`/360 MHz 构建。
