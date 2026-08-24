# Audio Playback

[English Version](README.md)

通过 Platform ES8311 输出 codec 和板载 speaker 播放音频。

## 要求

- Arduino-ESP32 `3.3.11`、`ESP32P4 Dev Module`、已启用 PSRAM，以及默认
  `ChipVariant=postv3`（v3.00+、400 MHz）。
- Platform ES8311 输出 codec 和板载 speaker。

## 构建

在 Arduino IDE 中打开 `Audio_Playback.ino`，或使用仓库文档中的 FQBN 和 board
options 编译。

## 预期结果

sketch 为 Platform 输出路径初始化播放。它不是麦克风示例：LCD-5 的 ES7210
`Mic_Record` 拓扑尚未移植到这个 ES8311 产品 surface。编译不能证明真实可听播放。

## HIL 清单

关闭串口 monitor 后冷启动，确认板载 speaker 可听到播放；随后以 `115200` 打开 UART0，
确认播放继续。

## 故障排查

确认所选开发板提供 ES8311 和 speaker 路径。pre-v3/v1.3 silicon 请单独显式使用
`ChipVariant=prev3`/360 MHz 构建。
