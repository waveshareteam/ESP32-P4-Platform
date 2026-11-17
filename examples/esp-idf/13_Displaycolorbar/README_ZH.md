# 显示色条

[English Version](./README.md)

在受支持的 LCD 面板上显示色条。这是本仓库中最简单的 ESP-IDF 显示 bring-up 示例。

## 难度

中级。

## 硬件要求

- ESP32-P4 开发板。
- 连接到预期接口的受支持 LCD 面板。

该示例使用 `waveshare/esp32_p4_platform` BSP 组件。对于该 BSP 支持的开发板，它是很好的第一个显示测试，因为应用不会创建 LVGL UI；它只初始化面板，并请求 DPI 面板驱动生成硬件垂直色条图案。

## 构建和烧录

```bash
cd examples/esp-idf/13_Displaycolorbar
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

## 预期行为

串口日志应显示：

```text
Initialize LCD device
Show color bar pattern drawn by hardware
```

LCD 应点亮背光并显示垂直色条。

## 排障

- 确认显示面板型号和接口。
- 如果面板路径需要 PSRAM，确认 PSRAM 已启用。
- 如果日志正常但屏幕是黑的，检查背光控制。
- 验证复位、电源和面板初始化设置。
- 在运行 [14_lvgl_demo_v9](../14_lvgl_demo_v9/) 或 [16_video_lcd_display](../16_video_lcd_display/) 前先运行此示例，把面板 bring-up 与 LVGL/摄像头处理分开测试。
