# Ethernet Basic

[English Version](./README.md)

启动以太网，等待链路连接，并获取 IP 地址。

## 难度

中级。

## 硬件要求

- 带以太网支持的 ESP32-P4 开发板，或受支持的外接以太网模块。
- 以太网线和带 DHCP 的网络。

## 构建和烧录

```bash
cd examples/esp-idf/11_ethernetbasic
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

## 配置

检查 **Example Configuration** 和以太网 PHY 设置：

- `Stop and deinit Ethernet after elapsing number of secs`：普通 bring-up 运行保持 `-1`，或设置一个值来测试驱动反初始化。
- 内部 EMAC 或 SPI Ethernet 模式。
- PHY 型号和 PHY 地址。
- 内部 EMAC 的 MDC/MDIO GPIO。
- 复位 GPIO。
- 时钟模式。

正确值取决于开发板原理图。

## 预期输出

日志应显示以太网驱动初始化、链路连接、以太网 MAC 地址和 DHCP 结果：

```text
Ethernet Started
Ethernet Link Up
Ethernet Got IP Address
ETHIP:...
```

在运行 [15_eth2ap](../15_eth2ap/) 前先使用此示例，因为它能在不加入 Wi-Fi AP 转发的情况下验证以太网侧。

## 排障

- 确认你的开发板确实有以太网硬件。
- 检查网线、交换机和 DHCP 服务器。
- 验证 PHY 地址和复位 GPIO。
- 如果链路一直无法连接，检查时钟模式和 PHY 供电。
