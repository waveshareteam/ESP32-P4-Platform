# eth-32 固件候选

[English](README.md) | [简体中文](README_ZH.md)

板卡为 ESP32-P4-ETH，适用 32 MB Flash、ESP32-P4 rev3.x（v3.0–v3.99）。使用 ESP-IDF v6.1.1 构建，供本地实物验证。

| Wi-Fi | Ethernet | 板载音频 | microSD | 小智 |
| --- | --- | --- | --- | --- |
| 无 | 有 | 有 | 有 | 有 |

按实际连接的屏幕选用固件，从 `0x0` 烧录。

| 屏幕 | 分辨率 | 文件 |
| --- | --- | --- |
| 7-DSI-TOUCH-A | 720 × 1280 | [ESP32-P4-Platform-eth-32-lcd7-a-rev3_x_FactoryOnly_20261118.bin](lcd7-a/ESP32-P4-Platform-eth-32-lcd7-a-rev3_x_FactoryOnly_20261118.bin) |
| 8-DSI-TOUCH-A | 800 × 1280 | [ESP32-P4-Platform-eth-32-lcd8-a-rev3_x_FactoryOnly_20261118.bin](lcd8-a/ESP32-P4-Platform-eth-32-lcd8-a-rev3_x_FactoryOnly_20261118.bin) |
| 10.1-DSI-TOUCH-A | 800 × 1280 | [ESP32-P4-Platform-eth-32-lcd10-1-a-rev3_x_FactoryOnly_20261118.bin](lcd10-1-a/ESP32-P4-Platform-eth-32-lcd10-1-a-rev3_x_FactoryOnly_20261118.bin) |

为 `PORT` 设置板卡串口，从此目录运行对应屏幕的命令。

完整出厂恢复可先执行 `esptool --chip esp32p4 --port "$PORT" erase-flash`。这会清除 Flash 中的设置和用户数据。

## 7-DSI-TOUCH-A

```bash
esptool --chip esp32p4 --port "$PORT" write-flash 0x0 lcd7-a/ESP32-P4-Platform-eth-32-lcd7-a-rev3_x_FactoryOnly_20261118.bin
```

## 8-DSI-TOUCH-A

```bash
esptool --chip esp32p4 --port "$PORT" write-flash 0x0 lcd8-a/ESP32-P4-Platform-eth-32-lcd8-a-rev3_x_FactoryOnly_20261118.bin
```

## 10.1-DSI-TOUCH-A

```bash
esptool --chip esp32p4 --port "$PORT" write-flash 0x0 lcd10-1-a/ESP32-P4-Platform-eth-32-lcd10-1-a-rev3_x_FactoryOnly_20261118.bin
```

[出厂程序源码](../../../../examples/factory/)。
