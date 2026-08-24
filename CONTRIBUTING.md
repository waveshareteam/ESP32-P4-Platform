# Contributing

[中文版本](CONTRIBUTING_CN.md)

Thanks for helping improve the ESP32-P4 Platform examples.

## Before You Start

- Search existing issues and pull requests to avoid duplicate work.
- Pick the smallest example or documentation area that demonstrates the
  problem or improvement.
- For hardware changes, note the exact board name and revision when possible.

## Development Setup

### ESP-IDF

Use an ESP-IDF version that matches the example you are changing. CI builds all
first-party ESP32-P4 examples with ESP-IDF `v5.5.5` and `v6.0.2`; an individual
example may declare a different minimum or an additional target-specific
configuration in its README and `idf_component.yml`.

Typical workflow:

```bash
cd examples/esp-idf/00_board_check
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

Use an external build directory if your local path is long or located on a
slow disk:

```bash
idf.py -B build-esp32p4 set-target esp32p4 build
```

### Arduino

For Arduino changes, read [examples/arduino/README.md](examples/arduino/README.md)
first. Include the Arduino-ESP32 core version and board selection in your pull
request description.

## Pull Request Checklist

Before opening a pull request:

- Build the changed example, or explain why it could not be built locally.
- Run the example on hardware when the change affects runtime behavior.
- Update README files when commands, dependencies, pins, or board support
  change.
- Keep generated files out of the commit, including `build/`, `sdkconfig`,
  `sdkconfig.old`, `managed_components/`, and `dependencies.lock`.
- Keep each pull request focused on one topic.

When adding a new example, use [docs/EXAMPLE_ROADMAP.md](docs/EXAMPLE_ROADMAP.md)
to choose a useful gap and [docs/PROJECT_STRUCTURE.md](docs/PROJECT_STRUCTURE.md)
for the expected layout.

## Commit Style

Use short, descriptive commit messages. Examples:

```text
fix(i2c): document pull-up requirements
docs(readme): add quick start commands
feat(display): add color bar panel option
```

## Reporting Issues

When opening an issue, include:

- Board name and revision.
- Example path.
- ESP-IDF or Arduino-ESP32 version.
- Host operating system.
- Full build error or sanitized serial log. Remove credentials, tokens, private
  network details, unique device identifiers, and local paths before posting.
- Photos or wiring notes when hardware setup matters.

## Code of Conduct

Participation in this project is covered by [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).
