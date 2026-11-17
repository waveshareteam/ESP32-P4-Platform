# Wi-Fi Station

[English Version](./README.md)

将 ESP32-P4 配置连接到现有 Wi-Fi 接入点。

## 难度

中级。

## 硬件要求

- 带 Wi-Fi 支持的开发板或配置。
- 2.4 GHz Wi-Fi 接入点。

某些 ESP32-P4 开发板需要外部或伴随 Wi-Fi 路径。使用此示例前请查看你的开发板文档。在本工程中，主组件清单引入了 `esp_wifi_remote` 和 `esp_hosted`，因此没有原生 Wi-Fi 的 ESP32-P4 配置必须先配置并连接好伴随 Wi-Fi 传输，Station 才能关联到接入点。

## 构建和烧录

请先使用 ESP-IDF 终端。如果你的编辑器插件无法构建该示例，先用命令行流程验证，再调试编辑器配置。

```bash
cd examples/esp-idf/10_wifistation
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

## 配置

在 `menuconfig` 中打开 **Example Connection Configuration** 或 **Example Configuration** 并设置：

- Wi-Fi SSID。
- Wi-Fi 密码。
- 如有需要，设置最大重试次数。
- 接受的认证模式。默认阈值是 WPA2-PSK。

源码会在连接成功后打印配置的 SSID 和密码。不要在共享串口日志中使用生产凭据。

## 预期输出

串口日志应显示 Wi-Fi 初始化、连接尝试，以及 DHCP 成功后的 IP 地址。

## 排障

- 确认开发板具备 Wi-Fi 支持。
- 确认 SSID 和密码。
- 使用 2.4 GHz 网络。
- 如果持续重试连接，将开发板移近接入点。
- 如果在编辑器扩展中构建失败，请改用 ESP-IDF 命令行 shell 重试，确保托管组件和目标相关工具已显式初始化。
