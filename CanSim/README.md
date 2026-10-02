# CanSim

`CanSim` is the C++ CAN telemetry simulator. It generates fake vehicle state, encodes that state into CAN frames, and writes the frames to a selectable output.

## What Lives Here

| Path | Purpose |
| --- | --- |
| `src/` | Simulator implementation and CLI entry point. |
| `include/cansim/` | Public simulator headers. |
| `tests/` | Unit and smoke tests for the simulator. |
| `config/can-signals.yaml` | Current CAN message and signal map. |
| `CMakeLists.txt` | Simulator build, tests, warnings, and format check. |

## Current Outputs

- `console`: prints CAN frames to stdout.
- `socketcan`: sends frames to a Linux SocketCAN interface.
- `recording`: stores frames in memory for tests.

## Build From Repository Root

```bash
cmake -S . -B build
cmake --build build --parallel
```

The root build keeps the simulator executable at:

```bash
./build/cansim
```

## Common Commands

Run defaults:

```bash
./build/cansim
```

Run one cruise frame set:

```bash
./build/cansim --scenario cruise --frames 1 --output console
```

Run scheduled output:

```bash
./build/cansim --scenario cruise --duration 10 --output console
```

Run against Linux SocketCAN:

```bash
./build/cansim --scenario cruise --frames 1 --output socketcan --interface vcan0
```

## Signal Map

The simulator uses `config/can-signals.yaml` when run from this folder and falls back to `CanSim/config/can-signals.yaml` when run from the repository root.

The map currently defines:

- `0x100` `VehicleMotion`
- `0x101` `EngineStatus`
- `0x102` `PowerStatus`
- `0x103` `GearStatus`
- `0x700` `Diagnostics`

Pass `--signal-map PATH` to test another message definition file.

## Tests

From the repository root:

```bash
ctest --test-dir build -R cansim --output-on-failure
```

The simulator tests cover CLI parsing, scenario defaults, byte-level encoding, output adapters, scheduling, signal-map parsing, and CLI smoke behavior.
