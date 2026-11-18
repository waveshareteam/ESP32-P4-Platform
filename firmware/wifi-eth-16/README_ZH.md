# wifi-eth-16 出厂固件

[English](README.md) | [简体中文](README_ZH.md)

本组固件适用于 ESP32-P4-NANO、ESP32-P4-Module-DEV-KIT 和 ESP32-P4-WIFI6-DEV-KIT（均为 16 MB Flash），要求芯片修订版为 ESP32-P4 rev3.x（v3.0 至 v3.99），使用 ESP-IDF v6.1.1 构建。

| Wi-Fi | Ethernet | 板载音频 | microSD | 小智 |
| --- | --- | --- | --- | --- |
| 有 | 有 | 有 | 有 | 有 |

## 固件下载

按实际连接的屏幕完整型号下载对应的合并 BIN：

| 屏幕 | 分辨率 | 文件 |
| --- | --- | --- |
| 7-DSI-TOUCH-A | 720 × 1280 | [ESP32-P4-Platform-7-DSI-TOUCH-A-wifi-eth-16-rev3-x_FactoryOnly_20261118.bin](ESP32-P4-Platform-7-DSI-TOUCH-A-wifi-eth-16-rev3-x_FactoryOnly_20261118.bin) |
| 8-DSI-TOUCH-A | 800 × 1280 | [ESP32-P4-Platform-8-DSI-TOUCH-A-wifi-eth-16-rev3-x_Factory_20261118.bin](ESP32-P4-Platform-8-DSI-TOUCH-A-wifi-eth-16-rev3-x_Factory_20261118.bin) |
| 10.1-DSI-TOUCH-A | 800 × 1280 | [ESP32-P4-Platform-10.1-DSI-TOUCH-A-wifi-eth-16-rev3-x_Factory_20261118.bin](ESP32-P4-Platform-10.1-DSI-TOUCH-A-wifi-eth-16-rev3-x_Factory_20261118.bin) |

## 烧录步骤

烧录或整片擦除会覆盖 Flash 中原有的应用、设置和用户数据，操作前请先按需备份。

1. 在 Bash 中进入 BIN 所在目录。
2. 将 `PORT` 设为实际串口设备名：

   ```bash
   PORT='<串口设备名>'
   ```

3. （可选）如需完整恢复出厂状态，擦除整片 Flash：

   ```bash
   esptool --chip esp32p4 --port "$PORT" erase-flash
   ```

4. 在同一 Bash 会话中，按实际连接的屏幕型号从 `0x0` 烧录对应固件：

### 7-DSI-TOUCH-A

```bash
esptool --chip esp32p4 --port "$PORT" write-flash 0x0 ESP32-P4-Platform-7-DSI-TOUCH-A-wifi-eth-16-rev3-x_FactoryOnly_20261118.bin
```

### 8-DSI-TOUCH-A

```bash
esptool --chip esp32p4 --port "$PORT" write-flash 0x0 ESP32-P4-Platform-8-DSI-TOUCH-A-wifi-eth-16-rev3-x_Factory_20261118.bin
```

### 10.1-DSI-TOUCH-A

```bash
esptool --chip esp32p4 --port "$PORT" write-flash 0x0 ESP32-P4-Platform-10.1-DSI-TOUCH-A-wifi-eth-16-rev3-x_Factory_20261118.bin
```

## 相关链接

- [出厂程序源码](../../examples/factory/README_ZH.md)
- [出厂固件总览](../README_ZH.md)
