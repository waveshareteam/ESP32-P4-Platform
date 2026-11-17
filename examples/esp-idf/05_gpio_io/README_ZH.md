# GPIO 输入输出

[English Version](./README.md)

这是一个小型 GPIO 输入/输出示例。

它每秒翻转一次输出引脚，并读取一个输入引脚。如果把输出引脚连接到输入引脚，输入值应跟随输出值变化。

## 难度

入门。

## 硬件要求

- 一块 ESP32-P4 开发板。
- 可选：一根连接已配置输出和输入引脚的跳线。
- 可选：如果想看到可视化输出，可使用带限流电阻的 LED。

## 默认引脚

| 信号 | 默认 GPIO |
| --- | --- |
| 输出 | GPIO2 |
| 输入 | GPIO3 |

如果这些引脚与你的开发板冲突，请在 `idf.py menuconfig` 中修改。

## 构建和烧录

```bash
cd examples/esp-idf/05_gpio_io
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

## 预期输出

```text
output=1 input=1
output=0 input=0
```

如果没有连接跳线，由于默认启用了输入上拉，输入可能会保持高电平。

## 可复用内容

- 使用 `gpio_config()` 配置输入和输出。
- 使用 `gpio_set_level()` 控制输出。
- 使用 `gpio_get_level()` 读取输入。
- 使用 `menuconfig` 选项做板级引脚选择。
