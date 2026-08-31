# ESP32-P4 Arduino Examples

[中文版本](./README_CN.md)

[![Arduino examples](https://github.com/waveshareteam/ESP32-P4-Platform/actions/workflows/arduino-examples.yml/badge.svg)](https://github.com/waveshareteam/ESP32-P4-Platform/actions/workflows/arduino-examples.yml)

These Arduino examples are mainly display-oriented. Most of them require:

- Arduino-ESP32 core
  [`3.3.11`](https://github.com/espressif/arduino-esp32/releases/tag/3.3.11).
- Board: `ESP32P4 Dev Module` (`esp32:esp32:esp32p4`).
- Default CI profile: `ChipVariant=postv3` (v3.00+, 400 MHz).
- Legacy/pre-v3 boards, including v1.3, must explicitly use
  `ChipVariant=prev3` (360 MHz); outputs from the two variants cannot be mixed.
- PSRAM enabled in the Arduino board settings.
- 16 MB flash with the `3MB APP/9.9MB FATFS` partition scheme for the
  repository CI baseline.
- A supported DSI display configuration selected through `CURRENT_SCREEN`.

For the full repository learning path, see [../README.md](../README.md) and
[../../docs/GETTING_STARTED.md](../../docs/GETTING_STARTED.md).

## Arduino-ESP32 Core

The repository CI pins Arduino-ESP32 `3.3.11` so every sketch is checked
against a reproducible toolchain. The corresponding Arduino CLI settings are:

```text
FQBN: esp32:esp32:esp32p4
Board options: PSRAM=enabled,USBMode=default,ChipVariant=postv3,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB
```

The normal Arduino matrix compiles the nine sketches once and packages each
successful build for this exact v3.00+ profile. It does not create a separate
revision-specific package. For pre-v3 silicon, use an explicit `prev3` build
and keep its output separate. A successful sketch compile or package validation
does not prove the connected board or peripherals are electrically compatible.

## Segmented Firmware Artifacts

Each successful workflow entry creates one source-built diagnostic ZIP. The
archive contains only the actual segments named by Arduino-ESP32's generated
`flash_args`, together with:

- `manifest.json`, with the exact FQBN, board options, product Git SHA, target,
  repository-relative sketch identity, application basename, compile identity,
  segment offsets, sizes, and SHA-256 values;
- `metadata/flash_args.source.txt`, the validated core-generated source, and a
  root `flash_args` rewritten only to the packaged `bin/` paths;
- `metadata/build_identity.json`, a canonical copy of the public build identity;
- `SHA256SUMS.txt`, `flash.sh`, and `flash.cmd`;
- the bootloader, partition table, `boot_app0` when generated, application, and
  any other segment required by that build under `bin/`.

The packager derives offsets and flash capacity from the build metadata and
rejects overlapping, out-of-range, missing, or modified segments. It does not
put an Arduino `*.merged.bin` in the ZIP and does not offer a whole-flash
single-file command. A bootloader may legitimately start at `0x0` on a target
whose real build metadata says so; the validator rejects the merged/whole-flash
case rather than banning that valid bootloader offset.

The provenance chain is fail closed: the declared repository sketch must match
`build.options.json`'s `sketchLocation`; its directory-named primary `.ino`, the
single generated `.ino.cpp` compile entry, the command's actual input, generated
object output, source-line content, the application binary basename, expanded upload recipe,
generated `flash_args`, and packaged segment bytes must all agree. The packager
derives its trusted repository root from its own Git checkout, requires a clean
worktree, and requires the full 40-character product SHA to equal that
checkout's `HEAD`; full archive validation repeats the current-repository binding
and requires the exact local build directory, expanded board properties, and
its local verbose compile log. Independently, it recomputes the build identity, replays the
generated-sketch compile, Arduino link (including the ELF and map), and esptool
`elf2image` command, checks the application with `image-info`, and compares every
packaged segment byte-for-byte with that build. An archive without that local
build evidence can be checked for ordinary checksums, but cannot receive a
source-to-binary provenance verdict.

Host-specific raw metadata is local evidence, not a public artifact. The ZIP
does not contain raw `build.options.json`, expanded board properties,
`compile_commands.json`, the verbose compile log, or generated translation-unit
metadata with absolute paths. It records only safe repository-relative identity
plus each raw file's basename, size, and SHA-256 after rechecking those files
against the trusted local build. Any home, temporary, workspace, user, or
tool-cache path makes packaging fail, including a path embedded in a binary
segment or ZIP metadata.
Compilation maps checkout, build, home, and Arduino tool-cache prefixes to
stable public labels before the package-wide privacy scan.

Regression tests independently remove or alter the POSIX and Windows helpers,
their segment entries, offsets, file names, and flash options. Every tampered
ZIP must still be rejected after its checksum file is regenerated. The package's
`SHA256SUMS.txt` provides checksum coverage, not a publisher signature; trusted
provenance comes from comparison with the exact local build under the clean
current checkout. This is static package validation for both helpers; `flash.cmd`
coverage is limited to static member-content and ZIP-tampering checks. CI does
not claim that it was executed on Windows or that either helper was tested on
hardware.

After extracting a package, verify it and flash its listed split files:

```text
python -m pip install -r requirements.txt
sha256sum -c SHA256SUMS.txt
./flash.sh PORT
```

On Windows, use `flash.cmd PORT`. Replace `PORT` with the board's programming
port. Both helpers run the copyable command in the manifest's
`write_flash_command` field, using the canonical esptool v5 `write-flash`
spelling. They reject an unverified esptool version before accessing the port.
Do not substitute remembered offsets: the included `flash_args` and manifest
are authoritative for that package.

The manifest records `segmented_payload_total`, the complete copyable segmented
write command, the full product Git SHA, and the task-time Registry BSP
component, version, content hash, and upstream repository commit. It explicitly
marks `used_by_build: false` because these Arduino builds do not link the
ESP-IDF BSP component.

## Serial Startup and Hardware-in-the-Loop Check

The repository profile keeps `USB CDC On Boot` disabled, so Arduino-ESP32 maps
`Serial` to UART0 for these builds. On every board in the
[product catalog](../../README.md), use the connector identified
by its official hardware guide as the Type-C UART/programming/debugging port;
the board schematics implement that path with a CH343P USB-to-UART bridge. Do
not use a separately labelled USB, USB OTG, or native USB connector as this
profile's log port. For example, the official
[ESP32-P4-NANO FAQ](https://docs.waveshare.com/ESP32-P4-NANO/FAQ) explicitly
maps its CH343 bridge to ESP32-P4 UART0. Native USB paths are not enabled or
HIL-covered by this matrix. The sketches never wait for a serial monitor or DTR
before starting. Their shared log initializer contains a
compile-time CDC guard that requests zero-timeout writes whenever CDC-on-boot
is enabled. Only the repository's actual UART0 profile is part of this build
matrix; the CDC guard is a code-level regression boundary, not a claim that a
different USB profile has passed hardware validation.

Compilation and package checks cannot prove cold-start behavior. Before a
candidate is accepted, repeat this checklist for every packaged sketch on the
matching board and display:

1. Close every serial monitor, power the board fully off, then power it on.
2. Confirm the sketch reaches its normal display, touch, or network behavior
   without opening a monitor.
3. Open the product's UART0 monitor at `115200` after normal operation begins.
4. Confirm that opening, closing, and reopening the monitor neither resets nor
   stalls the application, and that any available logs match the board design.
5. Record the board/revision, product Git SHA, package SHA-256, and result.

Until those checks pass on hardware, the generated ZIPs remain HIL candidates,
not factory or production firmware.

### Documentation

You can use the [Arduino-ESP32 Online Documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/) to get all information about this project.

## Bundled Dependencies

Use the libraries included in [`libraries/`](libraries/) rather than installing
different versions from the Arduino Library Manager. The bundled
`lv_conf.h` is also required by the LVGL sketch.

### [lvgl v9.3.0](https://github.com/lvgl/lvgl)

<p align="center">
  <img src="https://lvgl.io/github-assets/logo-colored.png" width=300px>
</p>

  <h1 align="center">Light and Versatile Graphics Library</h1>
  <br>
<div align="center">
  <img src="https://lvgl.io/github-assets/smartwatch-demo.gif">
  &nbsp;
  <img border="1px" src="https://lvgl.io/github-assets/widgets-demo.gif">
</div>

### [Arduino_GFX v1.6.0](https://github.com/moononournation/Arduino_GFX)

Arduino GFX provides the encapsulated ESP32-P4 MIPI DSI function

The `displays/` helper combines Waveshare panel/I2C integration code with
Arduino-adapted Espressif `esp_lcd_touch` and GT911 sources. The repository and
upstream portions are Apache-2.0 licensed; see the
[third-party inventory](../../THIRD_PARTY.md) for the pinned provenance.

## GT911 Touch Contract

The product examples use polling; they do not assign GT911 INT or RST GPIOs.
Before creating the touch IO/driver, they probe the shared I2C bus at `0x5D`
and then `0x14`, and initialize with the address that responds. If neither
address responds, the sketch continues safely without touch input. This matches
the ESP-IDF examples. It is not a claim that a fixed address or LCD-5 wiring
can be copied to every Platform board.

## Example Sketches

| Example | Difficulty | Hardware |
| --- | --- | --- |
| [HelloWorld](examples/HelloWorld/) | Beginner | DSI display |
| [AsciiTable](examples/AsciiTable/) | Beginner | DSI display |
| [Drawing_board](examples/Drawing_board/) | Intermediate | DSI touch display |
| [GFX_ESPWiFiAnalyzer](examples/GFX_ESPWiFiAnalyzer/) | Intermediate | DSI display and Wi-Fi support |
| [LVGLV9_Arduino](examples/LVGLV9_Arduino/) | Advanced | DSI touch display |
| [Camera_Preview](examples/Camera_Preview/) | Intermediate | OV5647 camera, PSRAM, DSI display, and the Platform I2C bus |
| [Camera_ISP_Tuning](examples/Camera_ISP_Tuning/) | Advanced | OV5647 camera, PSRAM, DSI display, and the Platform I2C bus |
| [SD_Card](examples/SD_Card/) | Intermediate | Inserted SD card |
| [Audio_Playback](examples/Audio_Playback/) | Intermediate | ES8311 codec and on-board speaker |

`Camera_Preview` and `Camera_ISP_Tuning` require the supported OV5647 module,
PSRAM, and the product's shared Platform I2C bus; they do not introduce a
separate camera-control bus. `SD_Card` needs an inserted card. `Audio_Playback`
targets the Platform ES8311 output codec and on-board speaker. LCD-5's ES7210
`Mic_Record` example is intentionally not a Platform example: its input-codec
topology differs from the Platform ES8311 surface.

## Continuous Integration

The [`Arduino examples` workflow](../../.github/workflows/arduino-examples.yml)
discovers and independently compiles the nine first-party sketches below.
Library changes compile the full matrix; a sketch-only change compiles only
that sketch. Library-owned examples under `libraries/**/examples/` are
excluded.

The normal sketch matrix compiles and validates one segmented diagnostic
package per selected sketch. It does not validate peripherals on hardware,
produce a revision-specific P4 package, or include an ESP32-C6 image. The
resulting software still needs the cold-start/monitor HIL check above and a real
P4+C6 board test where Hosted Wi-Fi is used.

See the [CI guide](../../docs/CI.md) for the exact triggers and manual-run
selector.

## Special Points to Note

### I2C Drivers

In libraries, we provide a way to wrap i2c_master.h ourselves and then provide some basic functions. The main reason is that after the esp-idf update, arduino-esp32 v3.2.0 uses a new version of i2c_master driver also known as i2c driver_ng, which is not compatible with some older libraries, including but not limited to some sensor libraries, touch libraries, and extended IO libraries.
