# SD Card

[English Version](README.md)

使用 Platform Arduino 配置访问已插入的 SD 卡。

## 要求

- Arduino-ESP32 `3.3.11`、`ESP32P4 Dev Module`、已启用 PSRAM，以及默认
  `ChipVariant=postv3`（v3.00+、400 MHz）。
- 已插入且受支持的 SD 卡。

## 构建

在 Arduino IDE 中打开 `SD_Card.ino`，或使用仓库文档中的 FQBN 和 board options
编译。

## 预期结果

sketch 报告卡初始化和基本存储操作。编译不能证明开发板上的卡插入、格式化、挂载或
数据完整性。

## HIL 清单

关闭串口 monitor 后冷启动，确认卡操作；随后以 `115200` 打开 UART0，确认正常操作
持续进行。

## 故障排查

确认已插入受支持且格式正确的卡。pre-v3/v1.3 开发板需要单独显式使用
`ChipVariant=prev3`/360 MHz 构建。
