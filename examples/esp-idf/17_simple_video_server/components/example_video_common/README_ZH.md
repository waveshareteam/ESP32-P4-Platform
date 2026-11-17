# Example Video Common Component

[English Version](./README.md)

该组件为 `esp_video` 提供板级初始化，包括 MIPI-CSI 传感器 I2C 端口、DVP 传感器 I2C 端口、DVP 接口配置和 SPI 接口配置。它主要用于 `esp_video` 示例，用来简化板级配置和初始化流程。

## 支持的开发板和 GPIO 引脚

| 硬件 | ESP32-P4-Function-EV-Board V1.4 | ESP32-P4-Function-EV-Board V1.5 | ESP32-P4-EYE | ESP32-S3-EYE |
|:-:|:-:|:-:|:-:|:-:|
| MIPI-CSI I2C SCL 引脚        |  8 |  8 | 13 | NA |
| MIPI-CSI I2C SDA 引脚        |  7 |  7 | 14 | NA |
| MIPI-CSI I2C Reset 引脚      | NA | NA | 26 | NA |
| MIPI-CSI I2C Power-down 引脚 | NA | NA | 12 | NA |
| MIPI-CSI I2C XCLK            | NA | NA | 11 | NA |
|   |   |   |   |
| DVP I2C SCL 引脚             | NA |  8 | NA |  5 |
| DVP I2C SDA 引脚             | NA |  7 | NA |  4 |
| DVP I2C Reset 引脚           | NA | 36 | NA | NA |
| DVP I2C Power-down 引脚      | NA | 38 | NA | NA |
| DVP XCLK 引脚                | NA | 20 | NA | 15 |
| DVP PCLK 引脚                | NA |  4 | NA | 13 |
| DVP V-SYNC 引脚              | NA | 37 | NA |  6 |
| DVP DE 引脚                  | NA | 22 | NA |  7 |
| DVP D0 引脚                  | NA |  2 | NA | 11 |
| DVP D1 引脚                  | NA | 32 | NA |  9 |
| DVP D2 引脚                  | NA | 33 | NA |  8 |
| DVP D3 引脚                  | NA | 23 | NA | 10 |
| DVP D4 引脚                  | NA |  3 | NA | 12 |
| DVP D5 引脚                  | NA |  6 | NA | 18 |
| DVP D6 引脚                  | NA |  5 | NA | 17 |
| DVP D7 引脚                  | NA | 21 | NA | 16 |
|   |   |   |   |
| SPI I2C SCL 引脚             | NA |  8 | NA |  5 |
| SPI I2C SDA 引脚             | NA |  7 | NA |  4 |
| SPI I2C Reset 引脚           | NA | NA | NA | NA |
| SPI I2C Power-down 引脚      | NA | NA | NA | NA |
| SPI XCLK 引脚                | NA | 20 | NA | 15 |
| SPI CS 引脚                  | NA | 37 | NA |  6 |
| SPI SCLK 引脚                | NA |  4 | NA | 13 |
| SPI Data0 I/O 引脚           | NA | 21 | NA | 16 |

**注意：** ESP32-P4-Function-EV v1.4 开发板和 ESP32-P4-EYE 默认不支持 DVP 接口摄像头传感器。如果需要把 DVP 接口摄像头传感器连接到这些开发板，请在菜单中选择 "Customized Development Board"，并根据你的具体硬件配置 GPIO 引脚和时钟。

### 自定义开发板默认配置

使用 "ESP32-XX-DevKitC" 开发板时，可以尝试下表所示的 "Customized Development Board" 默认 GPIO 引脚配置：

| 硬件 | ESP32-P4 | ESP32-S3 | ESP32-C3 | ESP32-C6 | ESP32-C61 | ESP32-C5 |
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
| MIPI-CSI CAM I2C SCL 引脚        |  8 | NA | NA | NA | NA | NA |
| MIPI-CSI CAM I2C SDA 引脚        |  7 | NA | NA | NA | NA | NA |
| MIPI-CSI CAM I2C Reset 引脚      | NA | NA | NA | NA | NA | NA |
| MIPI-CSI CAM I2C Power-down 引脚 | NA | NA | NA | NA | NA | NA |
| MIPI-CSI CAM I2C XCLK            | NA | NA | NA | NA | NA | NA |
|   |   |   |   |
| DVP CAM I2C SCL 引脚             |  8 |  5 | NA | NA | NA | NA |
| DVP CAM I2C SDA 引脚             |  7 |  4 | NA | NA | NA | NA |
| DVP CAM I2C Reset 引脚           | NA | NA | NA | NA | NA | NA |
| DVP CAM I2C Power-down 引脚      | NA | NA | NA | NA | NA | NA |
| DVP CAM XCLK 引脚                | 20 | 15 | NA | NA | NA | NA |
| DVP CAM PCLK 引脚                |  4 | 13 | NA | NA | NA | NA |
| DVP CAM V-SYNC 引脚              | 37 |  6 | NA | NA | NA | NA |
| DVP CAM DE 引脚                  | 22 |  7 | NA | NA | NA | NA |
| DVP CAM D0 引脚                  |  2 | 11 | NA | NA | NA | NA |
| DVP CAM D1 引脚                  | 32 |  9 | NA | NA | NA | NA |
| DVP CAM D2 引脚                  | 33 |  8 | NA | NA | NA | NA |
| DVP CAM D3 引脚                  | 23 | 10 | NA | NA | NA | NA |
| DVP CAM D4 引脚                  |  3 | 12 | NA | NA | NA | NA |
| DVP CAM D5 引脚                  |  6 | 18 | NA | NA | NA | NA |
| DVP CAM D6 引脚                  |  5 | 17 | NA | NA | NA | NA |
| DVP CAM D7 引脚                  | 21 | 16 | NA | NA | NA | NA |
|   |   |   |   |
| SPI CAM0 I2C SCL 引脚            |  8 |  5 |  5 |  5 |  5 |  5 |
| SPI CAM0 I2C SDA 引脚            |  7 |  4 |  4 |  4 |  4 |  4 |
| SPI CAM0 I2C Reset 引脚          | NA | NA | NA | NA | NA | NA |
| SPI CAM0 I2C Power-down 引脚     | NA | NA | NA | NA | NA | NA |
| SPI CAM0 XCLK 引脚               | 20 | 15 |  8 |  0 |  0 |  0 |
| SPI CAM0 CS 引脚                 | 37 |  6 | 10 |  1 |  8 | 10 |
| SPI CAM0 SCLK 引脚               |  4 | 13 |  6 |  6 |  6 |  6 |
| SPI CAM0 Data0 I/O 引脚          | 21 | 16 |  7 |  7 |  7 |  7 |
|   |   |   |   |
| SPI CAM1 I2C SCL 引脚            |  5 |  1 | NA | NA | NA | NA |
| SPI CAM1 I2C SDA 引脚            |  6 |  2 | NA | NA | NA | NA |
| SPI CAM1 I2C Reset 引脚          | NA | NA | NA | NA | NA | NA |
| SPI CAM1 I2C Power-down 引脚     | NA | NA | NA | NA | NA | NA |
| SPI CAM1 XCLK 引脚               | 23 | 39 | NA | NA | NA | NA |
| SPI CAM1 CS 引脚                 | 38 | 42 | NA | NA | NA | NA |
| SPI CAM1 SCLK 引脚               | 22 | 41 | NA | NA | NA | NA |
| SPI CAM1 Data0 I/O 引脚          |  3 | 40 | NA | NA | NA | NA |

**注意 1**：只有拥有两个以上 SPI 端口的 SoC（不包括 SPI flash 和 SPI RAM 端口）才能同时使用 SPI CAM0 和 SPI CAM1。

**注意 2**：如果 SPI CAM0 和 SPI CAM1 使用相同摄像头传感器，且该传感器只有一个 I2C 从机地址，则 SPI CAM0 和 SPI CAM1 应使用不同的 I2C 端口与目标摄像头传感器通信。

## 使用说明

### MIPI-CSI 开发套件

如需把开发板配置为 MIPI-CSI 接口，请按以下步骤操作：

```text
Example Video Initialization Configuration  --->
    Select Target Development Board (ESP32-P4-Function-EV-Board V1.5)
        ( ) ESP32-P4-Function-EV-Board V1.4
        (X) ESP32-P4-Function-EV-Board V1.5
        ( ) ESP32-P4-EYE

        ......

        ( ) Customized Development Board
    Select and Set Camera Sensor Interface  --->
        [*] MIPI-CSI  --->
        [ ] DVP  ----
```

如果你的开发板未列在菜单中，请选择 "Customized Development Board"，并根据开发板规格配置 GPIO 引脚：

```text
Example Video Initialization Configuration  --->
    Select Target Development Board (Customized Development Board)
        ( ) ESP32-P4-Function-EV-Board V1.4
        ( ) ESP32-P4-Function-EV-Board V1.5
        ( ) ESP32-P4-EYE

        ......

        (X) Customized Development Board
    Select and Set Camera Sensor Interface  --->
        [*] MIPI-CSI  --->
            (0) SCCB(I2C) Port Number
            (100000) SCCB(I2C) Frequency (100K-400K Hz)
            (8) SCCB(I2C) SCL Pin
            (7) SCCB(I2C) SDA Pin
            (-1) Reset Pin
            (-1) Power Down Pin
            (-1) XCLK Pin
        [ ] DVP  ----
```

### DVP 开发套件

如需把开发板配置为 DVP 接口，请按以下步骤操作：

```text
Example Video Initialization Configuration  --->
    Select Target Development Board (ESP32-P4-Function-EV-Board V1.5)
        ( ) ESP32-P4-Function-EV-Board V1.4
        (X) ESP32-P4-Function-EV-Board V1.5
        ( ) ESP32-P4-EYE

        ......

        ( ) Customized Development Board
    Select and Set Camera Sensor Interface  --->
        [ ] MIPI-CSI  ---
        [*] DVP  ---->
```

如果你的开发板未列在菜单中，请选择 "Customized Development Board"，并根据开发板规格配置 GPIO 引脚：

```text
Example Video Initialization Configuration  --->
    Select Target Development Board (Customized Development Board)
        ( ) ESP32-P4-Function-EV-Board V1.4
        ( ) ESP32-P4-Function-EV-Board V1.5
        ( ) ESP32-P4-EYE

        ......

        (X) Customized Development Board
    Select and Set Camera Sensor Interface  --->
        [ ] MIPI-CSI  ---
        [*] DVP  ---->
            (0) SCCB(I2C) Port Number
            (100000) SCCB(I2C) Frequency (100K-400K Hz)
            (20000000) XCLK Frequency (Hz)
            (8) SCCB(I2C) SCL Pin
            (7) SCCB(I2C) SDA Pin
            (-1) Reset Pin
            (-1) Power Down Pin
            (20) XCLK Pin
            (4) PCLK Pin
            (37) VSYNC Pin
            (22) DE Pin
            (2) Data0 Pin
            (32) Data1 Pin
            (33) Data2 Pin
            (23) Data3 Pin
            (3) Data4 Pin
            (6) Data5 Pin
            (5) Data6 Pin
            (21) Data7 Pin
```

### SPI 开发套件

如需启用 SPI video device 支持，请先启用以下配置：

```text
Component config  --->
    Espressif Video Configuration  --->
        [*] Enable SPI based Video Device  ----
```

然后把开发板配置为 SPI 接口：

```text
Example Video Initialization Configuration  --->
    Select Target Development Board (ESP32-P4-Function-EV-Board V1.5)
        ( ) ESP32-P4-Function-EV-Board V1.4
        (X) ESP32-P4-Function-EV-Board V1.5
        ( ) ESP32-P4-EYE

        ......

        ( ) Customized Development Board
    Select and Set Camera Sensor Interface  --->
        [ ] MIPI-CSI  ---
        [ ] DVP  ----
        [*] SPI  ---->
```

如果你的开发板未列在菜单中，请选择 "Customized Development Board"，并根据开发板规格配置 GPIO 引脚：

```text
Example Video Initialization Configuration  --->
    Select Target Development Board (Customized Development Board)
        ( ) ESP32-P4-Function-EV-Board V1.4
        ( ) ESP32-P4-Function-EV-Board V1.5
        ( ) ESP32-P4-EYE

        ......

        (X) Customized Development Board
    Select and Set Camera Sensor Interface  --->
        [ ] MIPI-CSI  ---
        [ ] DVP  ----
        [*] SPI  ---->
            ()  SCCB(I2C) Port Number
            (100000) SCCB(I2C) Frequency (100K-400K Hz)
            (2) SPI Port Number
                Select XCLK Source (ESP Clock Router)  --->
            (24000000) XCLK Frequency (Hz)
            (8) SCCB(I2C) SCL Pin
            (7) SCCB(I2C) SDA Pin
            (-1) Reset Pin
            (-1) Power Down Pin
            (20) XCLK Output Pin
            (37) Chip Select Pin
            (4) Clock Pin
            (21) Data0 I/O Pin
```

### 双 SPI 摄像头传感器

***ESP32-P4*** 和 ***ESP32-S3*** 拥有两个以上 SPI 端口（其中一个端口用于 SPI flash 和 SPIRAM），因此这两个 SoC 可以连接双 SPI 摄像头传感器。要启用双 SPI 摄像头传感器，请先启用以下配置：

```text
Component config  --->
    Espressif Video Configuration  --->
        [*] Enable SPI based Video Device  --->
            [*] Enable The Second SPI Video Device
```

然后选择 "Customized Development Board" 配置，因为当前没有专用的双 SPI 摄像头传感器开发板：

```text
Example Video Initialization Configuration  --->
    Select Target Development Board (Customized Development Board)  --->
        ......
        (X) Customized Development Board
```

根据你的开发板规格配置 GPIO 引脚：

```text
Example Video Initialization Configuration  --->
    Select and Set Camera Sensor Interface  --->
        [*] SPI Camera Sensor 0  --->
            (0) SCCB(I2C) Port Number
            (100000) SCCB(I2C) Frequency (100K-400K Hz)
            (2) SPI Port Number
                Select XCLK Source (ESP Clock Router)  --->
            (24000000) XCLK Frequency (Hz)
            (8) SCCB(I2C) SCL Pin (NEW)
            (7) SCCB(I2C) SDA Pin (NEW)
            (-1) Reset Pin (NEW)
            (-1) Power Down Pin (NEW)
            (20) XCLK Output Pin (NEW)
            (37) Chip Select Pin (NEW)
            (4) Clock Pin (NEW)
            (21) Data0 I/O Pin (NEW)
       [*] SPI Camera Sensor 1  --->
            (1) SCCB(I2C) Port Number
            (100000) SCCB(I2C) Frequency (100K-400K Hz)
            (1) SPI Port Number
                Select XCLK Source (LEDC Timer)  --->
            (20000000) XCLK Frequency (Hz)
            (1) XCLK LEDC Timer Number
            (1) XCLK LEDC Timer Channel
            (5) SCCB(I2C) SCL Pin (NEW)
            (6) SCCB(I2C) SDA Pin (NEW)
            (-1) Reset Pin (NEW)
            (-1) Power Down Pin (NEW)
            (23) XCLK Output Pin (NEW)
            (38) Chip Select Pin (NEW)
            (22) Clock Pin (NEW)
            (3) Data0 I/O Pin (NEW)
```

### 使用预初始化的 SCCB(I2C) 总线

某些开发板上的 MIPI-CSI、DVP、SPI 摄像头传感器或马达会共享同一个 SCCB(I2C) 端口和 GPIO 引脚，例如 ESP32-P4-Function-EV-Board V1.5。可以选择并设置以下选项：

```text
Example Video Initialization Configuration  --->
    [*] Use Pre-initialized SCCB(I2C) Bus for All Camera Sensors And Motors  --->
        (0) SCCB(I2C) Port Number
        (8) SCCB(I2C) SCL Pin
        (7) SCCB(I2C) SDA Pin
```
