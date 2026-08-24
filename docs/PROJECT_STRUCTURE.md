# Project Structure

[中文版本](./PROJECT_STRUCTURE_CN.md)

This repository is organized as a collection of examples rather than as one
monolithic firmware application.

## Top Level

| Path | Purpose |
| --- | --- |
| `README.md` | Project overview, quick start, board support, and compatibility matrix |
| `config/` | Shared ESP-IDF configuration overlays, including ESP32-P4 revision profiles |
| `examples/` | ESP-IDF and Arduino examples |
| `docs/` | Repository-level documentation |
| `.github/` | Issue and pull request templates |
| `CONTRIBUTING.md` | Contribution workflow |
| `SUPPORT.md` | Support channels |
| `SECURITY.md` | Actionable private vulnerability reporting policy |
| `THIRD_PARTY.md` | Third-party code inventory |
| `LICENSE.txt` | Apache-2.0 license |

## ESP-IDF Examples

Each directory under `examples/esp-idf/` is intended to build independently.
Common files include:

| File or directory | Purpose |
| --- | --- |
| `CMakeLists.txt` | ESP-IDF project entry point |
| `main/` | Application source code |
| `main/idf_component.yml` | Managed component dependencies, when needed |
| `components/` | Example-local components |
| `sdkconfig.defaults` | Default configuration committed to the repository |
| `sdkconfig.ci*` | Optional CI-oriented configuration overlays |
| `partitions.csv` | Example-specific partition table |
| `README.md` | Example-specific setup, build, and hardware notes |

Generated directories such as `build/`, `managed_components/`,
`dependencies.lock`, and local `sdkconfig` files should not be committed.
Chip revision choices that apply across examples should use the shared
overlays in `config/` rather than being copied into every `sdkconfig.defaults`
file.

## Arduino Examples

Arduino sketches live under `examples/arduino/examples/`. Bundled libraries
live under `examples/arduino/libraries/` so sketches can be opened and tested
with fewer manual dependency steps.

When adding Arduino content:

- Add the sketch under `examples/arduino/examples/<ExampleName>/`.
- Update `examples/README.md` and `examples/README_CN.md`.
- Update `examples/arduino/README.md` when the required Arduino core or
  bundled library version changes.
- Keep board pin assumptions documented in the sketch README or comments.

## Documentation Expectations

Every new hardware-facing example should document:

- Supported board or boards.
- Required peripherals, modules, cables, and jumpers.
- ESP-IDF or Arduino-ESP32 version expectations.
- Required `menuconfig` options or Arduino board settings.
- Build, flash, and monitor commands.
- Known limitations and troubleshooting notes.

For example coverage and suggested additions, see
[EXAMPLE_ROADMAP.md](EXAMPLE_ROADMAP.md).
