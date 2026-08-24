# ESP32-P4 Arduino 示例

[English Version](./README.md)

[![Arduino 示例](https://github.com/waveshareteam/ESP32-P4-Platform/actions/workflows/arduino-examples.yml/badge.svg)](https://github.com/waveshareteam/ESP32-P4-Platform/actions/workflows/arduino-examples.yml)

这些 Arduino 示例以显示应用为主，通常需要：

- Arduino-ESP32 core
  [`3.3.11`](https://github.com/espressif/arduino-esp32/releases/tag/3.3.11)。
- 开发板：`ESP32P4 Dev Module`（`esp32:esp32:esp32p4`）。
- 默认 CI profile：`ChipVariant=postv3`（v3.00+、400 MHz）。
- 包括 v1.3 在内的 legacy/pre-v3 开发板必须显式使用
  `ChipVariant=prev3`（360 MHz）；两种 variant 的输出不可混用。
- 在 Arduino 开发板设置中启用 PSRAM。
- 仓库 CI 基线使用 16 MB flash 和 `3MB APP/9.9MB FATFS` 分区表。
- 通过 `CURRENT_SCREEN` 选择受支持的 DSI 显示配置。

完整的仓库学习路径请参阅 [../README_CN.md](../README_CN.md) 和
[../../docs/GETTING_STARTED_CN.md](../../docs/GETTING_STARTED_CN.md)。

## Arduino-ESP32 Core

仓库 CI 固定使用 Arduino-ESP32 `3.3.11`，确保所有 sketch 使用可复现的
工具链进行检查。对应的 Arduino CLI 设置如下：

```text
FQBN: esp32:esp32:esp32p4
Board options: PSRAM=enabled,USBMode=default,ChipVariant=postv3,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB
```

普通 Arduino matrix 会编译 9 个 sketch 各一次，并为这个精确 v3.00+ profile 的每个
成功构建生成 package；它不会创建单独的芯片版本 package。pre-v3 silicon 请显式用
`prev3` 构建并保持输出独立。sketch 编译或 package 校验成功不能证明已连接开发板或
外设在电气上兼容。

## 分段固件 Artifact

工作流中的每个成功条目都会创建一个源码构建的诊断 ZIP。归档只包含
Arduino-ESP32 生成的 `flash_args` 实际列出的分段，并同时包含：

- `manifest.json`：记录精确 FQBN、board options、产品 Git SHA、target、仓库相对
  sketch 身份、application basename、compile identity，以及每段 offset、size 和
  SHA-256；
- `metadata/flash_args.source.txt`：经过校验的 core 原始输出；根目录中的
  `flash_args` 只把文件路径重写为 package 内的 `bin/` 路径；
- `metadata/build_identity.json`：公开 build identity 的 canonical 副本；
- `SHA256SUMS.txt`、`flash.sh` 和 `flash.cmd`；
- `bin/` 下的 bootloader、partition table、构建实际生成时的 `boot_app0`、
  application，以及工具链要求的其他真实分段。

打包器从构建元数据中解析 offset 和 flash 容量，并拒绝重叠、越界、缺失或被修改
的分段。ZIP 不包含 Arduino `*.merged.bin`，也不提供从 `0x0` 写入单个整片文件的
命令。某些 target 的真实构建元数据可能合法地把 bootloader 放在 `0x0`；校验器
拒绝的是 merged/whole-flash 情形，而不是这个合法 bootloader offset。

来源链会 fail closed：声明的仓库 sketch 必须与 `build.options.json` 的
`sketchLocation` 一致；以目录命名的主 `.ino`、唯一生成的 `.ino.cpp` compile entry、
compile command 的实际输入、生成文件与主源码逐行内容、application binary basename、
规范 object 输出、展开后的 upload recipe、生成的 `flash_args` 和 package 内分段字节必须全部相符。
打包器从自身 Git checkout 推导唯一可信仓库根目录，要求工作树 clean，并要求完整
40 字符产品 SHA 与该 checkout 的 `HEAD` 完全一致；完整 archive validator 会再次执行
当前仓库绑定，并强制提供精确本地 build 目录、展开后的 board properties 和本地 verbose
compile log；它会独立重新计算 build identity，依次重放生成 sketch 的编译、Arduino
链接（包括 ELF 与 map）和 esptool `elf2image` 命令，再用 `image-info` 检查 application，
最后将 package 中每个分段与该 build 逐字节比较。缺少该本地 build 证据时，只能检查
普通校验和，不能给出 source-to-binary provenance 通过结论。

带主机细节的原始 metadata 只作本地证据，不是公开 artifact。ZIP 不包含原始
`build.options.json`、展开的 board properties、`compile_commands.json`、verbose compile
log，也不包含带绝对路径的生成 translation-unit metadata；只记录安全的仓库相对
identity，以及各原始文件经可信本地 build 复核后的 basename、size 和 SHA-256。出现
home、临时、workspace、用户名或工具 cache 路径时，打包会直接失败，包括 binary 分段
或 ZIP metadata 中的路径。编译会先把 checkout、build、home 和 Arduino 工具 cache
前缀映射为稳定的公开标签，再执行整包隐私扫描。

回归测试会分别删除或篡改 POSIX 与 Windows helper、其中的分段条目、offset、
文件名和 flash option。即使重新生成 ZIP 校验和，所有被篡改的包仍必须被 package
validator 拒绝。`SHA256SUMS.txt` 只提供 checksum 覆盖，不是发布者签名；可信来源结论
来自 clean 当前 checkout 下精确本地 build 的逐项比较。这是两个 helper 的静态 package
验证；`flash.cmd` 覆盖仅包括静态成员内容和 ZIP 篡改检查。CI 不会宣称已在 Windows
执行它，也不会宣称任一 helper 已通过硬件测试。

解压 package 后，先校验再烧录其中列出的分段：

```text
python -m pip install -r requirements.txt
sha256sum -c SHA256SUMS.txt
./flash.sh PORT
```

Windows 使用 `flash.cmd PORT`。请把 `PORT` 替换为开发板编程端口。两个 helper
都会运行 manifest `write_flash_command` 字段中的可复制命令，并采用 esptool v5
的规范 `write-flash` 拼写；在访问端口前会拒绝未经验证的 esptool 版本。不要用记忆中
的通用 offset 替换；package 内的 `flash_args` 和 manifest 才是该包的依据。

manifest 会记录 `segmented_payload_total`、完整可复制的分段写入命令、完整产品 Git
SHA，以及任务执行时 Registry BSP 的 component、version、content hash 和上游
repository commit；同时明确标记 `used_by_build: false`，因为这些 Arduino 构建并
未链接 ESP-IDF BSP 组件。

## 串口启动与硬件在环检查

仓库 profile 保持 `USB CDC On Boot` 禁用，因此 Arduino-ESP32 在这些构建中把
`Serial` 映射到 UART0。对[产品目录](../../README_CN.md)中的每块板，都应
使用官方硬件说明标注的 Type-C UART/烧录/调试口；原理图显示该路径使用 CH343P
USB 转 UART。不要把另行标注的 USB、USB OTG 或原生 USB 口当成本 profile 的日志
口。例如官方 [ESP32-P4-NANO FAQ](https://docs.waveshare.com/ESP32-P4-NANO/FAQ)
明确把 CH343 bridge 映射到 ESP32-P4 UART0。本 matrix 没有启用或完成原生 USB
路径的 HIL。sketch 启动不会等待串口监视器或 DTR。共享日志初始化器包含编译期
CDC 防护：
只要构建启用 CDC-on-boot，就会请求零超时写入。本 matrix 只包含仓库实际使用的
UART0 profile；CDC 防护只是代码级回归边界，并不表示其他 USB profile 已通过
硬件验证。

编译和 package 校验不能证明真实冷启动。接受候选之前，应在匹配的开发板和显示屏
上对每个 package 重复以下步骤：

1. 关闭所有串口监视器，将开发板完全断电后重新上电。
2. 不打开监视器，确认 sketch 进入正常显示、触摸或网络功能。
3. 应用正常运行后，再以 `115200` 打开产品的 UART0 monitor。
4. 确认打开、关闭并重新打开 monitor 都不会让应用复位或卡死，并确认可见日志
   符合该板设计。
5. 记录开发板/硬件版本、产品 Git SHA、package SHA-256 和结果。

在这些硬件检查通过前，生成的 ZIP 只能作为 HIL 候选，不能视为出厂或量产固件。

Arduino-ESP32 的安装和开发说明请参阅
[在线文档](https://docs.espressif.com/projects/arduino-esp32/en/latest/)。

## 随附依赖

请使用 [`libraries/`](libraries/) 中随仓库提供的库，不要从 Arduino Library
Manager 安装其他版本。LVGL sketch 还需要其中的 `lv_conf.h`。

- [LVGL `9.3.0`](https://github.com/lvgl/lvgl)
- [Arduino_GFX `1.6.0`](https://github.com/moononournation/Arduino_GFX)
- `displays/` 中的 Waveshare 显示和 GT911 触摸辅助代码

`displays/` 辅助库由 Waveshare 面板/I2C 集成代码及适配 Arduino 的 Espressif
`esp_lcd_touch`、GT911 源码组成；仓库代码和上游部分均采用 Apache-2.0 许可证。
固定来源请参阅[第三方清单](../../THIRD_PARTY_CN.md)。

## GT911 触摸约定

产品示例使用轮询，不指定 GT911 的 INT 或 RST GPIO。创建 touch IO/driver 前，会在
共享 I2C bus 上先探测 `0x5D`、再探测 `0x14`，并使用实际响应地址初始化；两个地址
都未响应时，sketch 会安全地继续运行而不提供触摸输入。这与 ESP-IDF 示例一致，不能
据此复制固定地址或 LCD-5 的接线。

## 示例

| 示例 | 难度 | 硬件 |
| --- | --- | --- |
| [HelloWorld](examples/HelloWorld/) | 入门 | DSI 显示屏 |
| [AsciiTable](examples/AsciiTable/) | 入门 | DSI 显示屏 |
| [Drawing_board](examples/Drawing_board/) | 中级 | DSI 触摸显示屏 |
| [GFX_ESPWiFiAnalyzer](examples/GFX_ESPWiFiAnalyzer/) | 中级 | DSI 显示屏和 Wi‑Fi 支持 |
| [LVGLV9_Arduino](examples/LVGLV9_Arduino/) | 高级 | DSI 触摸显示屏 |
| [Camera_Preview](examples/Camera_Preview/) | 中级 | OV5647 摄像头、PSRAM、DSI 显示屏和 Platform I2C bus |
| [Camera_ISP_Tuning](examples/Camera_ISP_Tuning/) | 高级 | OV5647 摄像头、PSRAM、DSI 显示屏和 Platform I2C bus |
| [SD_Card](examples/SD_Card/) | 中级 | 已插入的 SD 卡 |
| [Audio_Playback](examples/Audio_Playback/) | 中级 | ES8311 codec 和板载 speaker |

`Camera_Preview` 和 `Camera_ISP_Tuning` 需要受支持的 OV5647、PSRAM，以及产品共用的
Platform I2C bus；它们不会另建 camera-control bus。`SD_Card` 需要插入卡。
`Audio_Playback` 面向 Platform 的 ES8311 输出 codec 和板载 speaker。LCD-5 的 ES7210
`Mic_Record` 没有移植到 Platform：其输入 codec 拓扑不同于 Platform ES8311 surface。

## 持续集成

[`Arduino examples` 工作流](../../.github/workflows/arduino-examples.yml)
会发现并独立编译上面的 9 个第一方 sketch。修改随附库时会编译完整矩阵；
只修改一个 sketch 时仅编译该 sketch。`libraries/**/examples/` 中第三方库
随附的示例不会进入矩阵。

普通 sketch matrix 会为每个被选择的 sketch 编译并校验一个分段诊断 package，
但不能代替硬件外设验证。它不会生成芯片版本专用的 P4 package，也不会包含
ESP32-C6 image。生成的软件仍须执行上面的冷启动/monitor HIL；使用 Hosted Wi-Fi
时还须在真实 P4+C6 开发板上验证。

工作流触发条件和手动选择方式请参阅[持续集成说明](../../docs/CI_CN.md)。

## 特别注意事项

### I2C 驱动

`libraries/` 中封装了 `i2c_master.h` 并提供基础函数。Arduino-ESP32 `3.2.0`
及更高版本基于新的 ESP-IDF I2C master driver（也称为 `driver_ng`），它与
部分旧版传感器、触摸和扩展 IO 库不兼容。移植其他库时，请确认其使用的是
当前 I2C 驱动 API。
