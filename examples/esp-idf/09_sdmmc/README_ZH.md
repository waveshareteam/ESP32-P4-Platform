# SDMMC

[English Version](./README.md)

通过 SDMMC 接口挂载并测试 SD 卡。

## 难度

中级。

## 硬件要求

- 带 SD 卡槽或已接线 SD 卡模块的 ESP32-P4 开发板。
- SD 卡。

## 构建和烧录

```bash
cd examples/esp-idf/09_sdmmc
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

## 配置

在 **SD/MMC Example Configuration** 中检查：

- 总线宽度：4-line 或 1-line 模式。
- CMD、CLK、D0、D1、D2、D3 GPIO。
- 格式化选项。
- SD 电源控制选项。

除非你愿意擦除卡上的内容，否则不要启用格式化选项。

默认 ESP32-P4 SDMMC 引脚：

| 信号 | 默认 GPIO |
| --- | --- |
| CLK | GPIO43 |
| CMD | GPIO44 |
| D0 | GPIO39 |
| D1 | GPIO40 |
| D2 | GPIO41 |
| D3 | GPIO42 |

默认总线宽度是 4-line 模式。如果使用手工接线或简单模块，请先切换到 1-line 模式，等卡能稳定挂载后再启用 4-line 模式。

## 预期输出

示例会挂载 SD 卡、打印卡信息、写入 `/sdcard/hello.txt`，将其重命名为 `/sdcard/foo.txt`，读回该文件，再写入 `/sdcard/nihao.txt`，读回后卸载卡。

## 排障

- 接线不确定时，先尝试 1-line 模式。
- 如果开发板使用卡检测或电源控制信号，检查相关接线。
- 使用一张已格式化为 FAT 的可靠 SD 卡。
- 使用外接模块时，SDMMC 走线尽量短。
- 示例为了方便启用了内部上拉，但真实硬件中的 SD 卡总线仍需要合适的外部上拉。
- 在使用内部 LDO 给 SD 供电的 ESP32-P4 开发板上，启用该选项前请根据原理图确认配置的 LDO ID。
