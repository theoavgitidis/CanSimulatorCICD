# CAN Telemetry Simulator Architecture

## Status

Planned.

## Context

The project is a C++ vehicle telemetry simulator with a strong CI/CD focus. The simulator emits fake car data and exposes it in ways that are useful for software testing, CLI-driven development, later button-based control, and microcontroller display experiments.

The current direction is to continue with CAN as the primary vehicle-facing interface.

The future real-world target path is:

```text
Car -> Vehicle CAN bus -> Microcontroller CAN interface -> Display
```

The simulator exists to stand in for the car during development:

```text
C++ Simulator -> vcan0 or USB-CAN adapter -> Microcontroller CAN interface -> Display
```

## Decisions

### 1. CAN is the primary simulator output

The simulator will generate realistic vehicle telemetry and encode it into CAN frames.

Initial signals:

- Vehicle speed
- Engine RPM
- Engine temperature
- Fuel level
- Battery voltage
- Throttle position
- Brake state
- Gear
- Diagnostic trouble codes

The simulator should model vehicle state internally, then translate that state into CAN frames through a dedicated encoder layer.

```text
Scenario Engine
      |
Vehicle State Model
      |
CAN Signal Encoder
      |
CAN Output Adapter
```

### 2. SocketCAN is the first CAN backend

Linux SocketCAN will be the first supported CAN interface because it gives us two important modes:

- `vcan` for local development and CI testing without hardware
- Real CAN hardware through USB-CAN adapters on Linux

Example development flow:

```text
C++ Simulator -> vcan0 -> test tools / integration tests
```

Example hardware flow:

```text
C++ Simulator -> can0 -> USB-CAN adapter -> CAN transceiver -> microcontroller
```

### 3. Microcontroller and display reachability

A microcontroller can receive simulated data through a real CAN bus and use it to drive a display.

Expected simulator-to-hardware development path:

```text
Computer running simulator
      |
USB-CAN adapter
      |
CANH / CANL
      |
CAN transceiver
      |
Microcontroller CAN controller
      |
Display
```

Expected final vehicle path:

```text
Car CAN bus
      |
CAN transceiver
      |
Microcontroller CAN controller
      |
Display
```

Good target boards:

- ESP32 with TWAI CAN controller plus external CAN transceiver
- STM32 board with built-in CAN controller plus transceiver
- Arduino-style board with an MCP2515 CAN module

The microcontroller does not call the simulator directly. It listens for CAN frames as if they came from a real vehicle bus. In the final setup, the real car replaces the simulator as the CAN frame source.

### 4. Virtual CAN enables CI/CD testing

The CI pipeline should test CAN behavior without requiring physical hardware.

Planned CI checks:

- Build the C++ simulator with CMake
- Run unit tests for vehicle state transitions
- Run unit tests for CAN frame encoding
- Bring up a virtual CAN interface
- Run integration tests against `vcan0`
- Verify expected CAN IDs and payload values
- Run formatting and static analysis checks

Linux CI jobs can use `vcan` for integration tests. macOS and Windows jobs can still build and run non-SocketCAN unit tests.

### 5. CAN message definitions are versioned project artifacts

The project should keep CAN signal definitions in a versioned file instead of scattering IDs and bit layouts throughout the code.

Planned format:

- Start with a simple project-owned YAML or JSON signal map
- Optionally add DBC import/export later

Example:

```yaml
messages:
  - name: VehicleSpeed
    id: 0x100
    cycle_ms: 100
    signals:
      - name: speed_kph
        start_bit: 0
        length: 16
        scale: 0.01
        offset: 0
        unit: kph
```

This keeps the simulator, tests, documentation, and microcontroller examples aligned.

### 6. CLI is the first control surface

The first implementation should be controlled through a command-line interface. A later control surface can be button-based, but it does not require a web framework.

Expected CLI responsibilities:

- Start and stop simulation
- Select driving scenario
- Inject faults
- Adjust signal values manually
- Display connection status for `vcan0` or real CAN interfaces
- Print current vehicle state
- Print emitted CAN frames when requested

Preferred control direction:

```text
CLI Commands
      |
Simulator Control Interface
      |
C++ Simulator Runtime
```

CAN remains the vehicle-facing output. CLI commands are for humans, local development, and CI smoke tests. A later button control surface should call the same simulator control interface rather than duplicating simulator logic.

## Initial CAN Frame Plan

| CAN ID | Message | Signals | Period |
| --- | --- | --- | --- |
| `0x100` | VehicleMotion | speed, throttle, brake | 100 ms |
| `0x101` | EngineStatus | rpm, engine temperature | 100 ms |
| `0x102` | PowerStatus | fuel level, battery voltage | 500 ms |
| `0x103` | GearStatus | selected gear | 100 ms |
| `0x700` | Diagnostics | active fault code summary | 1000 ms |

## Open Questions

- Should the first implementation use a custom signal map only, or should DBC support be included from the beginning?
- Which hardware target should we document first: ESP32, STM32, or Arduino with MCP2515?
- Should fault injection emit only diagnostic frames, or should it also alter normal telemetry values?
- Should the later button control surface be a small local desktop UI, physical hardware buttons, or both?

## Next Planned Step

Create the repository skeleton:

- CMake-based C++ project
- Core vehicle state model
- CAN signal encoder
- CLI control surface
- SocketCAN output adapter
- Unit tests for frame encoding
- Linux `vcan` integration test
