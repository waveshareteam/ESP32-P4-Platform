# UART 回环

[English Version](./README.md)

这是一个小型 UART 示例，从 TX 发送文本并在 RX 读回。

## 难度

入门到中级。

## 硬件要求

- 一块 ESP32-P4 开发板。
- 一根连接已配置 TX 和 RX 引脚的跳线。

## 默认引脚

| 信号 | 默认 GPIO |
| --- | --- |
| TX | GPIO43 |
| RX | GPIO44 |

如果这些引脚与你的开发板冲突，请在 `idf.py menuconfig` 中修改。

## 配置说明

默认 UART 端口是 UART1，波特率为 115200。它和用于烧录/日志的 USB 串口监视器是分开的，因此回环跳线必须连接在已配置的 TX 和 RX GPIO 之间。

仅使用 3.3 V UART 接线。做板级调试时，保持跳线较短，并避免使用已经被显示屏、SD 卡、摄像头或调试口占用的 GPIO。

## 构建和烧录

```bash
cd examples/esp-idf/07_uart_loopback
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

## 预期输出

TX 连接到 RX 时：

```text
received 33 bytes: hello from esp32-p4 uart loopback
```

没有跳线时，示例会打印警告。

## 可复用内容

- `uart_driver_install()`
- `uart_param_config()`
- `uart_set_pin()`
- `uart_write_bytes()` 和 `uart_read_bytes()`
- 用于发现接线或引脚选择错误的简单超时检查。
