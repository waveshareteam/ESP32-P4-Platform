# ESP32-P4 Revision Configuration

[中文版本](./ESP32P4_REVISION_CONFIG_CN.md)

This repository maintains two mutually incompatible ESP32-P4 product profiles.
They are selected by the revision configuration supplied to a product build; do
not reuse generated output from one profile for the other.

## Product Profiles

| Profile | Chip-revision settings | Intended silicon | Notes |
| --- | --- | --- | --- |
| `rev3_x` (default) | `CONFIG_ESP32P4_SELECTS_REV_LESS_V3=n`; `CONFIG_ESP32P4_REV_MIN_300=y` | ESP32-P4 rev v3.0 and later | Default for production v3.x silicon. |
| `rev1_3` (legacy, pre-v3) | `CONFIG_ESP32P4_SELECTS_REV_LESS_V3=y`; `CONFIG_ESP32P4_REV_MIN_100=y` | ESP32-P4 revisions below v3.0, including v1.3 | ESP-IDF has no official `v1.3`-specific Kconfig; `MIN_100` covers the v1.x family. |

The default profile is `rev3_x`. `rev1_3` remains an explicit legacy/pre-v3
compatibility profile; it is not a claim that ESP-IDF Kconfig can identify an
individual v1.3 stepping.

## Separate Build Outputs

The two profiles have separate build directories, generated `sdkconfig` files,
binaries, and CI artifacts. They are not interchangeable. When changing a
profile in a local checkout, use a separate build directory or remove the old
generated configuration before rebuilding; an existing `sdkconfig` can override
the defaults you intended to select.

Choose the profile that matches the silicon: `rev3_x` for v3.0 and later, or
explicit `rev1_3` for pre-v3 silicon including v1.3. The product flasher probes
the silicon revision and rejects a selected-profile mismatch. That silicon
check does not establish PCB, display, power, peripheral, or other electrical
compatibility for a particular board.

For Arduino-ESP32 `3.3.11`, the matching default is
`ChipVariant=postv3` (v3.00+, 400 MHz). The explicit legacy choice is
`ChipVariant=prev3` (pre-v3, 360 MHz). Do not mix either Arduino build output
with the other ChipVariant, or with an ESP-IDF `sdkconfig`/binary from the
other revision profile.

## Example Coverage Boundary

The ESP-IDF example matrix is one default-profile matrix: 20 direct projects
on ESP-IDF `v5.5.5` and `v6.0.2`, using `rev3_x`. The Arduino matrix is nine
sketches on Arduino-ESP32 `3.3.11`, using `ChipVariant=postv3`; its segmented
diagnostic packages are candidates for that v3.00+ profile only. Examples are
not doubled by revision. The maintained firmware/Brookesia product jobs keep
both `rev1_3` and `rev3_x` profiles available.

## Evidence Boundary

Successful compilation or a passing CI job proves only the selected software
configuration compiled. It does not prove flashing, boot, display, touch,
camera, audio, network, or other hardware-in-the-loop behavior. Test the exact
board and connected hardware before treating a profile as hardware accepted.
