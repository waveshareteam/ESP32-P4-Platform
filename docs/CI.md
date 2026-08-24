# Continuous Integration

[中文版本](CI_CN.md)

This repository uses ESP-IDF examples, Arduino examples, firmware/Brookesia
product jobs, and a lightweight repository-policy workflow. Every workflow
checks out the pull-request head SHA, and concurrency cancels obsolete runs for
the same pull request or branch.

## Example Matrices

| Surface | Coverage | Revision policy |
| --- | --- | --- |
| ESP-IDF | 20 direct projects under `examples/esp-idf/`, on ESP-IDF `v5.5.5` and `v6.0.2` | Default `rev3_x` (`MIN_300`) |
| Arduino | 9 direct sketches under `examples/arduino/`, on Arduino-ESP32 `3.3.11` | Default `ChipVariant=postv3` (v3.00+, 400 MHz) |

Example coverage is deliberately not doubled by chip revision. Pre-v3/v1.3
compatibility remains an explicit `rev1_3`/`MIN_100` and Arduino `prev3`
(360 MHz) selection; generated `sdkconfig` files and binaries are not
interchangeable with their v3.x counterparts. A direct source
change selects the affected project or sketch; shared framework inputs select
the applicable full matrix. Markdown and documentation assets do not select an
example build, while `firmware/` changes are reported outside the default
example matrix.

## Arduino Segmented Artifacts

Every selected Arduino sketch is compiled and then packaged from its generated
Arduino-ESP32 `flash_args`. The workflow uploads only the validated segmented
ZIP and its standalone manifest. It does not upload the 16 MiB merged build
image. The ZIP contains the original offset metadata, checksums, split binaries,
and POSIX/Windows flash helpers for the manifest's `write_flash_command`;
archive and segment validation fails
closed on unsafe paths, overlap, capacity errors, or hash/size mismatch.

The artifact is tied to the exact checked-out product SHA, Arduino core, FQBN,
board options, and task-time BSP provenance. The manifest explicitly records
that Arduino does not link the ESP-IDF BSP. See the
[Arduino guide](../examples/arduino/README.md) for flashing and the required
cold-start/serial-monitor HIL checklist. No Arduino artifact is accepted as
factory firmware on compilation evidence alone.

The packager derives one trusted repository root from its own clean Git
checkout and requires the declared full product SHA to equal `HEAD`. It binds
the repository-relative sketch and primary source to Arduino's build options,
single generated translation unit and its actual compile command/source lines,
canonical object output, application basename, upload recipe,
`flash_args`, and segment bytes. Raw `build.options.json`, expanded properties,
`compile_commands.json`, and host-specific generated metadata remain local.
Public artifacts contain only a field-whitelisted canonical identity and each
raw evidence file's basename, size, and SHA-256; any host, user, workspace,
temporary, or cache path in ZIP metadata, text, or binary payloads fails closed.
The Arduino compile step maps checkout, build, home, and tool-cache prefixes to
stable public labels before that whole-artifact scan.

The validator requires the exact local Arduino build directory, expanded
board properties, and its local verbose compile log. Independently, it recomputes the build identity,
replays the generated-sketch compile, Arduino link (including the ELF and map),
and esptool `elf2image` command, checks the resulting image with `image-info`,
and compares every ZIP segment byte-for-byte with that build. The public
identity records only the compile log's basename, size, and SHA-256; the
host-specific log itself stays local. `SHA256SUMS.txt` is internal checksum
coverage, not a publisher signature; archive-only checks cannot claim
source-to-binary provenance without the matching local build evidence.

The POSIX and Windows flash helpers are both reconstructed and tamper-tested as
ZIP members. `flash.cmd` coverage is static member-content and ZIP-tampering
coverage only: CI does not claim that it was executed on a Windows host, or
that either helper was verified on hardware.

## Firmware/Brookesia Product Jobs

The maintained firmware/Brookesia product jobs keep both `rev1_3` and `rev3_x`
profiles. They publish distinct artifacts named
`esp32-p4-platform-firmware-<profile>-<40-char SHA>`, retained for 14 days.
Each artifact is for 32 MiB flash, includes the split default image set with all
offsets, sizes, and SHA-256 values, and also includes a profile-matched
`merged.bin` as a non-default flashing method. It is not an image for a
non-default profile. The jobs do not produce an ESP32-C6 image and do not erase
hardware.

Generated build directories, `sdkconfig` files, binaries, and artifacts are
profile-specific and cannot be exchanged. The flasher probes silicon revision:
below v3.0 selects `rev1_3`, and v3.0 or later selects `rev3_x`; it rejects a
mismatch. A silicon revision alone does not prove that the PCB or attached
electrical hardware is compatible.

Checked-in factory firmware remains immutable, rev3.x-or-later, and 16 MiB. It
is a release surface distinct from the new 32 MiB CI product artifacts; no CI
job regenerates or replaces a factory package.

The product selector builds only non-documentation changes under
`firmware/brookesia/` and evidenced product build/packaging inputs. Known
example, policy, and documentation paths select no product build. An
unclassified non-document path fails closed and is reported as unknown instead
of silently spending two firmware builds or pretending the path is safe.

## Repository Policy

`Repository policy / result` is the always-visible static gate. It compiles all
Python files in `.github/scripts`, runs the full unittest discovery suite, then
runs the flasher PowerShell `-SelfTest` and `-ListOnly` parser paths without
normal flashing or hardware access. It also runs the strict Markdown ownership,
bilingual navigation, link, homepage, privacy/provenance, and changed-scope
audit. Unit coverage keeps CI routing behavior and the Arduino segmented-package
boundary checked; routing may also be audited with its dedicated CLI when a
complete base range is available.

The repository keeps its established public `_CN.md` companion URLs. Every
pair is explicit in `config/markdown-audit.json`; this is an intentional public
path compatibility policy, not an unclassified language suffix. The
repository-local gate is a tested target adaptation of the reusable audit: it
also recognizes linked product-catalog images as homepage imagery. The
multi-product profile therefore does not require a separate single-product
hero image. Synthetic tests cover both rules and the corresponding failure
cases.

## Evidence Boundary

Actions compilation proves the selected software configuration compiled; it does
not prove a flash, boot, display, touch, camera, audio, Hosted SDIO, Wi-Fi, or
other hardware-in-the-loop behavior. Hardware-sensitive changes require testing
on the matching board and connected devices. See
[ESP32-P4 Revision Config](ESP32P4_REVISION_CONFIG.md) and
[P4 + C6 Hosted Wi-Fi](P4_C6_HOSTED_WIFI.md) for their respective boundaries.
