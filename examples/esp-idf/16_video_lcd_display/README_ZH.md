| 支持目标 | ESP32-P4 |
| -------- | -------- |

[English Version](./README.md)

# Video LCD Display

该示例基于 [esp_video](https://github.com/espressif/esp-video-components/tree/master/esp_video) 组件，演示如何把摄像头图像显示到 LCD 屏幕上。应用会初始化 Waveshare ESP32-P4 platform BSP 显示，打开 MIPI-CSI video device，使用 PPA 进行缩放/旋转/镜像处理，并通过 LVGL adapter dummy-draw 路径把每帧 blit 到 LCD frame buffer。

## ESP-IDF 要求

- 该示例支持 ESP-IDF release/v5.4 及更高分支。
- 工程依赖 `esp_video` 和 `waveshare/esp32_p4_platform`。
- 请按照 [ESP-IDF 编程指南](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/index.html) 设置开发环境。**我们强烈建议**先 [构建第一个工程](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/index.html#build-your-first-project)，熟悉 ESP-IDF 并确保环境正确。

### 前置条件

* 一个受所选 BSP 显示路径支持的 ESP32-P4 开发板。
* 一个由 [EK79007](https://dl.espressif.com/dl/schematics/display_driver_chip_EK79007AD_datasheet.pdf) IC 驱动的 7 英寸 1024 x 600 LCD 屏幕，配套 32-pin FPC 连接 [adapter board](https://dl.espressif.com/dl/schematics/esp32-p4-function-ev-board-lcd-subboard-schematics.pdf)（[LCD Specifications](https://dl.espressif.com/dl/schematics/display_datasheet.pdf)）。
* 一个受 `esp_video` 支持的 MIPI-CSI 摄像头传感器。默认 `sdkconfig.defaults` 选择 OV5647，MIPI RAW8 `800 x 1280`，50 FPS。
* 用于供电和烧录的 USB-C 线。
* 请按以下步骤连接：
    * **Step 1**. 按下表把屏幕转接板背面的引脚连接到开发板对应引脚。

        | 屏幕转接板 | ESP32-P4-Function-EV-Board |
        | ---------- | -------------------------- |
        | 5V（任意一个） | 5V（任意一个） |
        | GND（任意一个） | GND（任意一个） |
        | PWM | GPIO26 |
        | LCD_RST | GPIO27 |

    * **Step 2**. 通过 `MIPI_DSI` 接口连接 LCD FPC。
    * **Step 3**. 通过 `MIPI_CSI` 接口连接 Camera FPC。
    * **Step 4**. 使用 USB-C 线连接 `USB-UART` 口到 PC（用于供电和查看串口输出）。
    * **Step 5**. 打开开发板电源开关。

### 配置工程

运行 `idf.py menuconfig`，配置 BSP 显示、摄像头传感器和视频流水线选项。

MIPI-CSI 摄像头默认 ESP32-P4 SCCB/I2C 引脚：

| 信号 | 默认 GPIO |
| --- | --- |
| SCL | GPIO8 |
| SDA | GPIO7 |

在 `Espressif Camera Sensors` 配置菜单中，选择与你的硬件匹配的摄像头传感器。当前默认值为：

```text
Component config  --->
    Espressif Camera Sensors Configurations  --->
        [*] OV5647  ---->
            Default format select for MIPI  --->
                (X) RAW8 800x1280 50fps, MIPI input
```

如果使用 SC2336 或其他传感器，请把传感器选择和输出格式改成与摄像头模块匹配。

### 构建和烧录

构建工程并烧录到开发板，然后运行监视工具查看串口输出（将 `PORT` 替换为开发板串口名）：

```bash
idf.py set-target esp32p4
idf.py -p PORT flash monitor
```

输入 `Ctrl-]` 退出串口监视器。

完整配置和使用 ESP-IDF 构建工程的步骤，请参见 [ESP-IDF 入门指南](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/get-started/index.html)。

### 预期行为

显示背光点亮，LCD 显示实时摄像头图像。日志会打印 video driver 版本、设备名、总线信息，以及检测到的帧宽高。帧分配在 PSRAM 中，并复制到 `CONFIG_BSP_LCD_DPI_BUFFER_NUMS=3` 配置的三重 LCD frame buffer。

### 排障

- 先运行 [13_Displaycolorbar](../13_Displaycolorbar/) 验证 LCD 路径。
- 如果出现 `video cam open failed`，检查摄像头 FPC 方向、传感器供电、SCCB/I2C 引脚和已选传感器型号。
- 确认 PSRAM 已启用且稳定；摄像头 buffer 会从 PSRAM 分配。
- 如果图像裁剪、镜像或旋转不正确，请调整传感器输出格式或 `main/main.c` 中的 PPA 操作。
