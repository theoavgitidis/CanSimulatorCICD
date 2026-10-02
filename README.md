# CAN Simulator and Display CI/CD

Vehicle telemetry tooling for two related codebases:

- `CanSim`: a C++ CAN simulator that emits fake vehicle telemetry.
- `CanDisplay`: the beginning of the ESP32-S3 display receiver firmware path.

The simulator stands in for a car while the receiver and display code are being built and tested.

## Target System

Final vehicle path:

```text
Car
  |
Vehicle CAN bus
  |
ESP32-S3 CAN interface
  |
Display
```

Development and CI path:

```text
CanSim
  |
console, vcan0, or USB-CAN adapter
  |
CanDisplay decoder / future ESP32-S3 firmware
  |
Display
```

## Repository Layout

| Path | Purpose |
| --- | --- |
| `CanSim/` | Simulator CLI, CAN encoding, signal map, output adapters, and simulator tests. |
| `CanDisplay/` | Display-side receiver code, starting with a hardware-independent CAN decoder. |
| `docs/hardware/` | ESP32-S3, CAN transceiver, wiring, and bench test notes. |
| `docs/plannedArchitectures/` | Architecture and CI/CD planning diagrams. |
| `.github/workflows/ci.yml` | GitHub Actions build, test, format, and SocketCAN smoke workflow. |
| `HANDOFF.md` | Fast project-resume notes. |
| `TODO.md` | Current implementation backlog. |

## Current Status

Implemented:

- Root CMake project with separate `CanSim` and `CanDisplay` subprojects.
- Simulator CLI executable: `./build/cansim`.
- Simulator CAN encoders for `0x100`, `0x101`, `0x102`, `0x103`, and `0x700`.
- YAML-driven simulator signal map at `CanSim/config/can-signals.yaml`.
- Console, recording, and Linux SocketCAN output adapters.
- Timed repeated sending with per-message periods from the signal map.
- `idle`, `cruise`, and `braking` simulator scenarios.
- Display-side `candisplay::CanFrameView`, `TelemetryState`, and `decodeCanFrame()`.
- Unit, smoke, and formatting tests registered with CTest.
- GitHub Actions CI with Linux `vcan0` SocketCAN smoke coverage.

Not implemented yet:

- Time-varying simulator scenarios.
- Named fault injection such as `engine_overheat`.
- `--print-state`, `--list-scenarios`, and `--list-messages`.
- ESP32-S3 TWAI receive loop.
- Real display rendering code.
- USB-CAN adapter bench validation.

## Build And Test

From the repository root:

```bash
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Format all C++ sources:

```bash
cmake --build build --target format
```

Run only formatting checks:

```bash
ctest --test-dir build -R "format_check" --output-on-failure
```

If `clang-format` is missing on macOS:

```bash
brew install clang-format
```

## Simulator Usage

Run the default simulator frame set:

```bash
./build/cansim
```

Run one frame set from a scenario:

```bash
./build/cansim --scenario cruise --frames 1 --output console
```

Run with manual values:

```bash
./build/cansim --speed 100 --throttle 50 --brake false --rpm 3000 --engine-temp 90 --fuel 75 --battery 12.6 --gear 3 --dtc 0x0420 --frames 1 --output console
```

Run for a timed duration using message periods from `CanSim/config/can-signals.yaml`:

```bash
./build/cansim --scenario cruise --duration 10 --output console
```

Override all message periods with a fixed cadence:

```bash
./build/cansim --scenario cruise --duration 1 --period-ms 100 --output console
```

Use an explicit signal map path:

```bash
./build/cansim --scenario cruise --duration 10 --signal-map CanSim/config/can-signals.yaml --output console
```

The default signal map path works when running from the repository root or from inside `CanSim/`.

## CAN Message Contract

The current simulator and display decoder share this message contract:

| CAN ID | Message | Signals | Period |
| --- | --- | --- | --- |
| `0x100` | `VehicleMotion` | speed, throttle, brake | 100 ms |
| `0x101` | `EngineStatus` | RPM, engine temperature | 100 ms |
| `0x102` | `PowerStatus` | fuel level, battery voltage | 500 ms |
| `0x103` | `GearStatus` | selected gear | 100 ms |
| `0x700` | `Diagnostics` | active fault flag, active DTC | 1000 ms |

Example output:

```text
0x100 [8] 10 27 32 00 00 00 00 00
0x101 [8] B8 0B 5A 00 00 00 00 00
0x102 [8] 4B EC 04 00 00 00 00 00
0x103 [8] 04 00 00 00 00 00 00 00
0x700 [8] 01 20 04 00 00 00 00 00
```

## SocketCAN

On Linux, the simulator can send frames to SocketCAN:

```bash
./build/cansim --scenario cruise --frames 1 --output socketcan --interface vcan0
```

Inspect frames with:

```bash
candump vcan0
```

On macOS, `--output socketcan` is parsed but not supported at runtime because SocketCAN is a Linux kernel API. Use console output locally, GitHub Actions, or a Linux VM for `vcan0` testing.

## Display Receiver Direction

`CanDisplay` is intentionally host-buildable first. The decoder can be tested before the ESP32-S3 arrives:

```text
CAN frame bytes
      |
candisplay::decodeCanFrame()
      |
candisplay::TelemetryState
      |
future ESP32 display renderer
```

Later ESP32-specific code should stay thin:

- Configure ESP-IDF TWAI or an Arduino/PlatformIO TWAI wrapper.
- Receive standard 11-bit CAN frames.
- Convert received frames into `candisplay::CanFrameView`.
- Update `candisplay::TelemetryState`.
- Render the latest telemetry state on the display.

## VS Code Setup

Recommended extensions:

- C/C++ by Microsoft
- CMake Tools by Microsoft

If VS Code marks project includes such as `#include "cansim/can_encoder.hpp"` or `#include "candisplay/can_decoder.hpp"` as errors:

1. `Cmd + Shift + P`
2. `CMake: Configure`
3. `C/C++: Reset IntelliSense Database`
4. `Developer: Reload Window`

The workspace settings point IntelliSense at both `CanSim/include` and `CanDisplay/include`.

## Documentation

- [CanSim project README](CanSim/README.md)
- [CanDisplay project README](CanDisplay/README.md)
- [Hardware notes](docs/hardware/README.md)
- [CAN telemetry architecture](docs/plannedArchitectures/can-telemetry-simulator.md)
- [CI/CD pipeline architecture](docs/plannedArchitectures/cicd-pipeline.md)
- [Architecture diagrams](docs/plannedArchitectures/architecture-diagrams.md)
- [Project handoff](HANDOFF.md)
- [Project TODOs](TODO.md)
