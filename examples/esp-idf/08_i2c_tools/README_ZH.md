# I2C 工具

[English Version](./README.md)

用于扫描和调试 I2C 总线设备的交互式 I2C 命令行工具。

## 难度

入门到中级。

## 硬件要求

- 一块 ESP32-P4 开发板。
- 可选：连接到已配置 SDA/SCL 引脚的 I2C 设备。
- 如果开发板或模块没有提供上拉电阻，需要外接上拉电阻。

即使没有外接 I2C 设备，示例也可以运行，但扫描不会发现设备。

## 构建和烧录

```bash
cd examples/esp-idf/08_i2c_tools
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

## 配置

在 `menuconfig` 中检查 **Example Configuration**：

- `SCL GPIO Num`
- `SDA GPIO Num`
- `Store command history in flash`

默认 ESP32-P4 引脚：

| 信号 | 默认 GPIO |
| --- | --- |
| SDA | GPIO7 |
| SCL | GPIO8 |

初始总线配置不会启用内部上拉，因此建议使用外部上拉或模块自带上拉，以获得可靠扫描结果。`i2cconfig` 命令可以在运行时重新配置总线，并为该运行时总线实例启用内部上拉。

启用命令历史时，示例会使用 `partitions_example.csv` 中的自定义 FAT 分区，把控制台历史保存到 flash。

## 常用命令

监视器打开后，在提示符中输入：

```text
i2c-tools> i2cdetect
i2c-tools> i2cconfig --sda 7 --scl 8 --freq 400000
i2c-tools> i2cget -c 0x50 -r 0x00 -l 1
i2c-tools> i2cset -c 0x50 -r 0x00 0x12
```

在控制台中使用 `help` 查看支持的命令和参数。

## 排障

- 如果所有地址都是空的，检查 SDA/SCL 引脚、电源、地线和上拉电阻。
- 如果一次出现很多地址，检查接线问题或缺失的上拉。
- 如果目标设备是 5 V，确认 I2C 引脚已经做电平转换。
- 如果扫描成功但 `i2cget` 或 `i2cset` 失败，先确认设备寄存器寻址是 8 位还是 16 位，再把命令复用到应用中。
