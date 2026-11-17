# Ethernet To AP

[English Version](./README.md)

结合以太网和 Wi-Fi AP 行为的网络示例。

## 难度

高级。

## 硬件要求

- 带以太网能力的 ESP32-P4 开发板，或受支持的以太网模块。
- Wi-Fi 支持路径。
- 以太网线和网络。

该示例在以太网和 Wi-Fi SoftAP 之间转发数据包。在本仓库的 ESP32-P4 配置中，工程使用 `esp_wifi_remote` 和 `esp_hosted` 提供 Wi-Fi 路径，因此在 AP 可以转发流量前，必须先准备好伴随 Wi-Fi 传输。

## 构建和烧录

请先使用 ESP-IDF 终端。如果你的编辑器插件无法构建该示例，先用命令行流程验证，再调试编辑器配置。

```bash
cd examples/esp-idf/15_eth2ap
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

## 配置

在 `menuconfig` 中检查以太网和 Wi-Fi AP 设置。

本工程默认 AP 设置：

| 设置 | 默认值 |
| --- | --- |
| SSID | `ESP32-P4-NANO-WiFi` |
| 密码 | `12345678` |
| 信道 | `1` |
| 最大 Station 数 | `4` |

`sdkconfig.defaults` 中默认 ESP32-P4 以太网引脚：

| 信号 | GPIO |
| --- | --- |
| MDC | GPIO31 |
| MDIO | GPIO52 |
| PHY reset | GPIO51 |

正确的以太网 PHY 型号、地址、复位、时钟和 SMI 值仍取决于开发板原理图。

## 预期行为

以太网链路连接后，示例会把以太网 MAC 地址复制到 SoftAP 接口并启动 Wi-Fi AP 模式。以太网链路断开后，它会停止 AP。客户端数据包通过内部流控队列转发，用于平衡以太网和 Wi-Fi 吞吐。

## 排障

- 先运行 [11_ethernetbasic](../11_ethernetbasic/)。
- 先运行 [10_wifistation](../10_wifistation/) 或其他 Wi-Fi 示例，验证 Wi-Fi 传输。
- 检查 IP 地址段和 DHCP 行为。
- 确认你的开发板支持所需的两条网络路径。
- 如果在编辑器扩展中构建失败，请改用 ESP-IDF 命令行 shell 重试，确保托管组件和目标相关工具已显式初始化。
