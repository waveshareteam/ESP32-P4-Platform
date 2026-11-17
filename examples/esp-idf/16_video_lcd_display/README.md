| Supported Targets | ESP32-P4 |
| ----------------- | -------- |

[中文版本](README_ZH.md)

# Video LCD Display

This example is based on the [esp_video](https://github.com/espressif/esp-video-components/tree/master/esp_video) component and demonstrates how to display images from the camera on an LCD screen. The application initializes the Waveshare ESP32-P4 platform BSP display, opens the MIPI-CSI video device, uses PPA scale/rotate/mirror processing, and blits each frame into the LCD frame buffer through the LVGL adapter dummy-draw path.

## ESP-IDF Required

- This example supports ESP-IDF release/v5.4 and later branches.
- The project depends on `esp_video` and `waveshare/esp32_p4_platform`.
- Please follow the [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html) to set up the development environment. **We highly recommend** you [Build Your First Project](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#build-your-first-project) to get familiar with ESP-IDF and make sure the environment is set up correctly.

### Prerequisites

* An ESP32-P4 board supported by the selected BSP display path.
* A 7-inch 1024 x 600 LCD screen powered by the [EK79007](https://dl.espressif.com/dl/schematics/display_driver_chip_EK79007AD_datasheet.pdf) IC, accompanied by a 32-pin FPC connection [adapter board](https://dl.espressif.com/dl/schematics/esp32-p4-function-ev-board-lcd-subboard-schematics.pdf) ([LCD Specifications](https://dl.espressif.com/dl/schematics/display_datasheet.pdf)).
* A MIPI-CSI camera sensor supported by `esp_video`. The default `sdkconfig.defaults` selects OV5647, MIPI RAW8 `800 x 1280` at 50 FPS.
* A USB-C cable for power supply and programming.
* Please refer to the following steps for the connection:
    * **Step 1**. According to the table below, connect the pins on the back of the screen adapter board to the corresponding pins on the development board.

        | Screen Adapter Board | ESP32-P4-Function-EV-Board |
        | -------------------- | -------------------------- |
        | 5V (any one)         | 5V (any one)               |
        | GND (any one)        | GND (any one)              |
        | PWM                  | GPIO26                     |
        | LCD_RST              | GPIO27                     |

    * **Step 2**. Connect the FPC of LCD through the `MIPI_DSI` interface.
    * **Step 3**. Connect the FPC of Camera through the `MIPI_CSI` interface.
    * **Step 4**. Use a USB-C cable to connect the `USB-UART` port to a PC (Used for power supply and viewing serial output).
    * **Step 5**. Turn on the power switch of the board.

### Configure the Project

Run `idf.py menuconfig` and configure the BSP display, camera sensor, and video pipeline options.

Default ESP32-P4 SCCB/I2C pins for the MIPI-CSI camera:

| Signal | Default GPIO |
| --- | --- |
| SCL | GPIO8 |
| SDA | GPIO7 |

In the `Espressif Camera Sensors` configuration menu, select the camera sensor that matches your hardware. The current defaults are:

```
Component config  --->
    Espressif Camera Sensors Configurations  --->
        [*] OV5647  ---->
            Default format select for MIPI  --->
                (X) RAW8 800x1280 50fps, MIPI input
```

If you use SC2336 or another sensor, update the sensor selection and output format to match the camera module.

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output (replace `PORT` with your board's serial port name):

```bash
idf.py set-target esp32p4
idf.py -p PORT flash monitor
```

To exit the serial monitor, type ``Ctrl-]``.

See the [ESP-IDF Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html) for full steps to configure and use ESP-IDF to build projects.

### Expected Behavior

The display backlight turns on and the LCD shows the live camera image. The log prints the video driver version, device name, bus information, and the detected frame width and height. Frames are allocated in PSRAM and copied into the triple LCD frame buffer configured by `CONFIG_BSP_LCD_DPI_BUFFER_NUMS=3`.

### Troubleshooting

- Run [13_Displaycolorbar](../13_Displaycolorbar/) first to verify the LCD path.
- Check camera FPC orientation, sensor power, SCCB/I2C pins, and selected sensor model if `video cam open failed` appears.
- Confirm PSRAM is enabled and stable; camera buffers are allocated from PSRAM.
- If the image is cropped, mirrored, or rotated incorrectly, adjust the sensor output format or the PPA operation in `main/main.c`.
