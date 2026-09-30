# CAN Simulator CI/CD

A planned C++ vehicle telemetry simulator focused on CAN output and CI/CD practice.

The simulator will generate fake vehicle telemetry, encode it into CAN frames, and send those frames first to the terminal, then to Linux SocketCAN interfaces such as `vcan0` and later real CAN hardware.

## Target End State

The final real-world system is:

```text
Car
  |
Vehicle CAN bus
  |
Microcontroller CAN interface
  |
Display
```

The simulator is a development and CI tool for the same microcontroller/display path:

```text
C++ CAN Simulator
  |
vcan0 or USB-CAN adapter
  |
Microcontroller CAN interface
  |
Display
```

In other words, the simulator stands in for the car while the microcontroller firmware and display behavior are being developed and tested.

## Current Status

This repository has a working first C++ CLI implementation slice.

Implemented so far:

- Architecture notes
- System diagram
- Class diagram
- CI/CD implementation diagram
- CMake project skeleton
- `CanFrame` type
- `VehicleState` type
- CAN encoders for `0x100`, `0x101`, `0x102`, `0x103`, and `0x700`
- CLI executable with arguments for speed, throttle, brake, RPM, temperature, fuel, battery, gear, diagnostics, and frame count
- CLI parsing module
- `encodeAll()` frame collection helper
- Basic `idle`, `cruise`, and `braking` scenarios
- Console and recording CAN output adapters
- Linux SocketCAN output adapter for `vcan0` and real CAN interfaces
- Versioned CAN signal map in `config/can-signals.yaml`
- CAN output factory for CLI-selected output creation
- CTest-registered encoder, CLI parser, scenario, output, smoke, and formatting tests
- GitHub Actions workflow

Not implemented yet:

- CAN signal map YAML loading
- Time-based scenario updates

## Project Goals

- Build a small but realistic C++ CAN telemetry simulator.
- Practice CI/CD with a project that has tests, formatting, static analysis, integration tests, and release artifacts.
- Support local development without CAN hardware through console output and Linux `vcan`.
- Later support real CAN hardware through SocketCAN and USB-CAN adapters.
- Keep the first control surface simple: command-line interface first, button-based control later.

## Planned Architecture

```text
CLI Commands
      |
Simulator Control Interface
      |
C++ Simulator Runtime
      |
CAN Signal Encoder
      |
CAN Output Adapter
```

Initial output adapters:

- Console output for local development and CI smoke tests
- Recording output for unit and integration tests
- SocketCAN output for Linux `vcan0` and real CAN interfaces

Later control option:

```text
Button Control Surface
      |
Simulator Control Interface
      |
C++ Simulator Runtime
```

## Planned CAN Messages

| CAN ID | Message | Signals | Period |
| --- | --- | --- | --- |
| `0x100` | VehicleMotion | speed, throttle, brake | 100 ms |
| `0x101` | EngineStatus | RPM, engine temperature | 100 ms |
| `0x102` | PowerStatus | fuel level, battery voltage | 500 ms |
| `0x103` | GearStatus | selected gear | 100 ms |
| `0x700` | Diagnostics | active fault code summary | 1000 ms |

## Current CLI Usage

Build the project:

```bash
cmake -S . -B build
cmake --build build
```

Run the simulator:

```bash
./build/cansim
```

Current output:

```text
0x100 [8] 10 27 32 00 00 00 00 00
0x101 [8] B8 0B 5A 00 00 00 00 00
0x102 [8] 4B EC 04 00 00 00 00 00
0x103 [8] 04 00 00 00 00 00 00 00
0x700 [8] 00 00 00 00 00 00 00 00
```

Run with simple CLI values:

```bash
./build/cansim --speed 100 --throttle 50 --brake false --rpm 3000 --engine-temp 90 --fuel 75 --battery 12.6 --gear 3 --dtc 0x0420 --frames 1 --output console
```

Run with a scenario:

```bash
./build/cansim --scenario cruise --frames 1
```

On Linux, send frames to SocketCAN:

```bash
./build/cansim --scenario cruise --frames 1 --output socketcan --interface vcan0
```

On macOS, `--output socketcan` is parsed but not supported at runtime because SocketCAN is a Linux kernel API. Use console output locally on your Mac, and use GitHub Actions or a Linux VM for `vcan0` integration testing.

Available scenarios:

- `idle`
- `cruise`
- `braking`

Manual values override scenario values:

```bash
./build/cansim --scenario cruise --speed 80 --gear 4 --frames 1
```

The diagnostics frame uses `00` when no fault is active. With `--dtc 0x0420`, it starts with:

```text
0x700 [8] 01 20 04 00 00 00 00 00
```

Run tests:

```bash
ctest --test-dir build --output-on-failure
```

Format C++ files:

```bash
cmake --build build --target format
```

Check formatting through CTest:

```bash
ctest --test-dir build -R cansim_format_check --output-on-failure
```

If `clang-format` is missing on macOS, install it with:

```bash
brew install clang-format
```

## Planned CLI Usage

These commands describe the intended interface. They will become valid once the simulator executable exists.

Print CAN frames to the terminal:

```bash
cansim --scenario cruise --frames 10 --output console
```

Print current vehicle state:

```bash
cansim --scenario idle --print-state
```

Inject a fault:

```bash
cansim --scenario cruise --fault engine_overheat
```

Send CAN frames to a Linux virtual CAN interface:

```bash
cansim --scenario cruise --output socketcan --interface vcan0
```

## Where CAN Output Will Appear

The first implementation will print CAN frames directly in the terminal:

```text
0x100 [8] 10 27 32 00 00 00 00 00
0x101 [8] B8 0B 5A 00 00 00 00 00
0x102 [8] 64 00 7E 04 00 00 00 00
```

On Linux, SocketCAN output can be inspected with:

```bash
candump vcan0
```

Example:

```text
vcan0  100   [8]  10 27 32 00 00 00 00 00
vcan0  101   [8]  B8 0B 5A 00 00 00 00 00
```

## Planned Build Flow

```bash
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build
```

## VS Code Setup

Install these extensions:

- C/C++ by Microsoft
- CMake Tools by Microsoft

If VS Code marks project includes such as `#include "cansim/can_encoder.hpp"` as errors, run:

1. `Cmd + Shift + P`
2. `CMake: Configure`
3. `C/C++: Reset IntelliSense Database`
4. `Developer: Reload Window`

The project exports CMake compile commands and includes `.vscode/settings.json` so the C/C++ extension can use CMake Tools as its configuration provider.

If the errors remain, check the bottom-right status bar in VS Code:

- CMake should show a selected kit.
- C++ should use the `Mac` configuration.
- The opened folder should be the repository root, not the `src/` or `include/` folder.

## CI/CD Direction

CI/CD should start as soon as the first compileable function and first unit test exist.

The first pipeline should:

- Configure the CMake project
- Build the simulator
- Run unit tests
- Run a small CLI smoke test once the executable exists
- Check formatting

Later pipeline stages should add:

- `clang-tidy`
- Sanitizer builds
- Linux `vcan0` integration tests
- macOS and Windows builds for platform-independent code
- Release artifacts for version tags

## Documentation

Planning documents:

- [CAN telemetry simulator architecture](docs/plannedArchitectures/can-telemetry-simulator.md)
- [CI/CD pipeline architecture](docs/plannedArchitectures/cicd-pipeline.md)
- [Architecture diagrams](docs/plannedArchitectures/architecture-diagrams.md)

## First Implementation Milestone

The first practical milestone is:

1. Create a CMake-based C++ project.
2. Add a small `VehicleState` model.
3. Add a `CanFrame` type.
4. Implement encoding for `0x100` and `0x101`.
5. Add unit tests for the encoder.
6. Add a CLI that prints frames to the terminal.
7. Add the first CI workflow that builds and runs the tests.
