# I2S Codec

[English Version](./README.md)

使用 I2S 的音频 codec 示例。根据配置，它可以运行在音乐播放模式或回声模式。

## 难度

中级。

## 硬件要求

- 带受支持音频 codec 路径的 ESP32-P4 开发板。
- 播放模式需要扬声器或耳机。
- 如果使用回声模式，需要麦克风输入。

## 构建和烧录

```bash
cd examples/esp-idf/12_I2SCodec
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

## 配置

在 **Example Configuration** 中检查：

- 模式：music 或 echo。
- 麦克风增益。
- 音量。
- BSP 支持选项。

`main/example_config.h` 使用的默认非 BSP ESP32-P4 接线：

| 信号 | 默认 GPIO |
| --- | --- |
| I2C SDA | GPIO7 |
| I2C SCL | GPIO8 |
| I2S MCLK | GPIO13 |
| I2S BCLK | GPIO12 |
| I2S WS | GPIO10 |
| I2S DOUT | GPIO9 |
| I2S DIN | GPIO11 |
| 功放使能 | GPIO53 |

只有在所选 BSP 与你的开发板音频原理图匹配时才启用 BSP 选项。当前 Kconfig 中，回声模式只在非 BSP 路径可用。

## 预期行为

- Music 模式播放内置的 `canon.pcm` 样本。
- Echo 模式把麦克风输入通过 codec 路径回放。
- 在音频播放或回声开始前，日志会打印 `i2s driver init success` 和 `es8311 codec init success`。

## 排障

- 确认 codec 供电、时钟、I2C 控制总线和 I2S 引脚。
- 从适中音量开始。
- 如果没有声音，检查所选开发板路径是否匹配你的硬件。
- 如果 codec 初始化失败，在相同 SDA/SCL 引脚上运行 [08_i2c_tools](../08_i2c_tools/) 确认 ES8311 是否响应控制总线。
