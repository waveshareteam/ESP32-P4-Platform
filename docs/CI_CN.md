# 持续集成

[English Version](CI.md)

本仓库使用 ESP-IDF 示例、Arduino 示例、firmware/Brookesia 产品 job，以及轻量
repository-policy 工作流。每条工作流都会检出 pull request 的 head SHA；并发设置
会取消同一 pull request 或分支上的陈旧运行。

## 示例 Matrix

| 范围 | 覆盖 | 芯片版本策略 |
| --- | --- | --- |
| ESP-IDF | `examples/esp-idf/` 下 20 个直属工程，使用 ESP-IDF `v5.5.5` 和 `v6.0.2` | 默认 `rev3_x`（`MIN_300`） |
| Arduino | `examples/arduino/` 下 9 个直属 sketch，使用 Arduino-ESP32 `3.3.11` | 默认 `ChipVariant=postv3`（v3.00+、400 MHz） |

示例覆盖刻意不按芯片版本翻倍。pre-v3/v1.3 兼容保持为显式的
`rev1_3`/`MIN_100` 与 Arduino `prev3`（360 MHz）选择；生成的 `sdkconfig` 和二进制
不可与 v3.x 对应输出混用。直属源码变更选择受影响的工程或 sketch；共享框架
输入选择相应的完整 matrix。Markdown 和文档资源不选择示例构建，而 `firmware/`
变更会被报告为默认示例 matrix 之外的内容。

## Arduino 分段 Artifact

每个被选择的 Arduino sketch 都会先编译，再根据 Arduino-ESP32 生成的
`flash_args` 打包。工作流只上传校验通过的分段 ZIP 和独立 manifest，不上传构建
产生的 16 MiB merged image。ZIP 包含原始 offset 元数据、校验和、分段 binary，
以及用于 manifest `write_flash_command` 的 POSIX/Windows 烧录 helper；遇到不安全
路径、分段重叠、容量错误或 hash/size 不一致时，归档与分段校验都会 fail closed。

artifact 会绑定精确 checkout 的产品 SHA、Arduino core、FQBN、board options，
以及任务执行时的 BSP 来源。manifest 会明确记录 Arduino 并未链接 ESP-IDF BSP。
烧录方法及必须执行的冷启动/串口监视器 HIL 清单请参阅
[Arduino 指南](../examples/arduino/README_CN.md)。不能只凭编译证据把任何 Arduino
artifact 视为出厂固件。

打包器从自身 clean Git checkout 推导唯一可信仓库根目录，并要求声明的完整产品 SHA
等于 `HEAD`。它会把仓库相对 sketch 与主源文件绑定到 Arduino build options、唯一
生成的 translation unit 及其实际 compile command/源码行、application basename、
规范 object 输出、upload recipe、`flash_args` 和分段字节。原始 `build.options.json`、展开 properties、
`compile_commands.json` 和带主机
信息的生成 metadata 只保留在本地；公开 artifact 只包含字段白名单的 canonical
identity，以及每个原始证据文件的 basename、size 和 SHA-256。任何主机、用户名、
workspace、临时目录或 cache 路径只要出现在 ZIP metadata、文本或 binary payload 中
都会 fail closed。Arduino 编译会先把 checkout、build、home 和工具 cache 前缀映射为
稳定的公开标签，再执行这项整包扫描。

validator 强制使用精确本地 Arduino build 目录、展开后的 board properties 和本地 verbose
compile log；它会独立重新计算 build identity，依次重放生成 sketch 的编译、Arduino
链接（包括 ELF 与 map）和 esptool `elf2image` 命令，再用 `image-info` 检查生成镜像，并把
ZIP 中每个分段与该 build 逐字节比较。公开 identity 只记录 compile log 的 basename、
size 和 SHA-256，带主机信息的原始日志仍只保留在本地。`SHA256SUMS.txt` 只提供包内
checksum 覆盖，不是发布者签名；没有匹配的本地 build 证据时，archive-only 检查不能
宣称 source-to-binary provenance 已通过。

POSIX 与 Windows 烧录 helper 都会作为 ZIP 成员被精确重建并接受篡改负例测试；`flash.cmd`
覆盖只包括静态成员内容和 ZIP 篡改覆盖。CI 不会宣称已在 Windows 主机执行它，也不会
宣称任一 helper 已通过硬件验证。

## Firmware/Brookesia 产品 Job

维护的 firmware/Brookesia 产品 job 保留 `rev1_3` 和 `rev3_x` 两个 profile。它们
发布独立 artifact，名称为
`esp32-p4-platform-firmware-<profile>-<40-char SHA>`，保留 14 天。每个 artifact
面向 32 MiB flash，包含带全部 offset、size 和 SHA-256 值的默认分段镜像集，并额外
包含与该 profile 匹配的 `merged.bin`，作为非默认烧录方式。它不是用于非默认 profile
的镜像。这些 job 不生成 ESP32-C6 image，也不擦除硬件。

生成的 build 目录、`sdkconfig`、二进制文件和 artifact 都是 profile 专属，不能
互换。flasher 会探测 silicon revision：低于 v3.0 时选择 `rev1_3`，v3.0 及以后
选择 `rev3_x`，并拒绝不匹配的 profile。但 silicon revision 本身不能证明 PCB 或
已连接电气硬件兼容。

已提交的 factory firmware 仍保持不可变、面向 rev3.x-or-later 且为 16 MiB。它是
独立发布表面，不同于新的 32 MiB CI 产品 artifact；没有 CI job 会重新生成或替换
factory package。

产品 selector 只会为 `firmware/brookesia/` 下的非文档变更和已证实的产品构建/打包
输入执行构建。已知的示例、策略和文档路径不会选择产品构建；未分类的非文档路径会
fail closed 并报告为 unknown，而不是静默浪费两次固件构建或假定该路径安全。

## Repository Policy

`Repository policy / result` 是始终可见的静态门禁。它会编译 `.github/scripts` 中
的全部 Python 文件，运行完整 unittest discovery 套件，然后运行 flasher 的
PowerShell `-SelfTest` 和 `-ListOnly` 解析路径，不会进行正常烧录或访问硬件。它还
会运行严格的 Markdown 所有权、双语导航、链接、主页、隐私/来源和变更范围审计。
单元测试会持续检查 CI 路由行为和 Arduino 分段 package 边界；当完整 base 范围
可用时，也可以使用专用 CLI 审计路由。

仓库保留既有的公开 `_CN.md` 中文配套页面 URL。所有配对均在
`config/markdown-audit.json` 中显式列出；这是有意的公开路径兼容策略，而不是未分类
的语言后缀。仓库本地门禁是可复用审计的带测试目标适配，还会把带产品链接的目录图
识别为主页图片。因此 multi-product profile 不要求额外的单产品 hero image；合成
测试覆盖这两条规则及相应失败情形。

## 证据边界

Actions 编译只能证明所选软件配置能够编译，不能证明烧录、启动、显示、触摸、
摄像头、音频、Hosted SDIO、Wi-Fi 或其他硬件在环行为。硬件敏感变更必须在匹配的
开发板和已连接设备上测试。相应边界请参阅
[ESP32-P4 版本配置](ESP32P4_REVISION_CONFIG_CN.md) 和
[P4 + C6 Hosted Wi-Fi](P4_C6_HOSTED_WIFI_CN.md)。
