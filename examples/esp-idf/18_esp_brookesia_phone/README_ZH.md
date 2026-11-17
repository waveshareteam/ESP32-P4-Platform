# ESP32-P4 Platform 运行 ESP-Brookesia Phone 示例

[English Version](./README.md)

这个示例演示了如何在 Waveshare ESP32-P4 Platform 显示 BSP 上运行 ESP-Brookesia Phone UI。它基于乐鑫 ESP-Brookesia Phone 示例改造，适配本仓库中的 ESP32-P4 Platform 开发板以及 Waveshare DSI 触摸屏。

工程使用本示例目录中已经包含的 ESP-Brookesia 和 BSP 组件，启动 BSP 显示，安装注册表中的 Phone 应用，并通过 LVGL 定时器更新状态栏时钟。

## 入门指南

### 硬件要求

* 一个受 `waveshare/esp32_p4_platform` 支持的 ESP32-P4 Platform 开发板。
* 一个在 BSP 菜单中选择的 Waveshare DSI 触摸屏。
* 已启用 PSRAM。

### ESP-IDF 要求

- 此示例支持 IDF release/v5.3 及更高的分支。
- 请按照 [ESP-IDF 编程指南](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/index.html) 设置开发环境。**我们强烈建议**您 [构建第一个项目](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/index.html#build-your-first-project)，以熟悉 ESP-IDF 并确保环境设置正确。

### 已包含组件

使用本仓库中的示例时，不需要再单独克隆 `esp-brookesia`。本应用使用：

- `components/brookesia_core`
- `components/brookesia_app_squareline_demo`
- 组件清单中的 `waveshare/esp32_p4_platform`

### 配置

运行 `idf.py menuconfig` 并修改 ESP-Brookesia 和 BSP 配置。

默认 `sdkconfig.defaults` 关闭了 AI framework、services、speaker system 以及部分 Boost 模块，让此示例专注于显示和触摸 UI。默认配置还选择 `CONFIG_BSP_LCD_TYPE_720_1280_7_INCH_A`，启用 LVGL 日志，并使用 3 个 LCD DPI buffer。

请在以下菜单中选择实际连接的屏幕：

```text
Board Support Package(ESP32-P4) -> Display -> Select LCD type
```

运行时，如果 BSP 分辨率匹配 `1024 x 600` 或 `800 x 1280`，应用会选择对应的深色样式表。

## 如何使用示例

### 构建和烧录示例

构建项目并将其烧录到开发板，然后运行监视工具查看串行输出（将 `PORT` 替换为您的开发板串口名称）：

```bash
idf.py set-target esp32p4
idf.py -p PORT flash monitor
```

要退出串行监视器，请输入 ``Ctrl-]``。

完整的配置和使用 ESP-IDF 构建项目的步骤，请参见 [ESP-IDF 入门指南](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/get-started/index.html)。

### 预期现象

串口日志会打印 `Display ESP-Brookesia phone demo`，显示背光点亮，屏幕出现 Phone UI。示例使用 `ESP_LV_ADAPTER_ROTATE_0`、三缓冲局部防撕裂模式和默认触摸轴设置启动 BSP 显示。

### 排障建议

- 先运行 [13_Displaycolorbar](../13_Displaycolorbar/) 验证 LCD 通路。
- 如需把 LVGL/BSP 显示问题和 Brookesia 问题分开，先运行 [14_lvgl_demo_v9](../14_lvgl_demo_v9/)。
- 如果 UI 正常但触摸方向或位置不对，检查 BSP 触摸控制器以及 `main/main.cpp` 中的 `touch_flags`。
- 如果在 ESP-IDF 5.5 下修改防撕裂模式，请查看 `main/main.cpp` 中关于三缓冲防撕裂修复的注释。

## 技术支持和反馈

请使用以下反馈渠道：

- 有技术问题，请访问 [esp32.com](https://esp32.com/viewforum.php?f=35) 论坛。
- 如需提交功能请求或错误报告，请创建 [GitHub issue](https://github.com/espressif/esp-brookesia/issues)。

我们会尽快回复您。
