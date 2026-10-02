# CAN Telemetry System Architecture

Last updated: 2026-10-02

## Status

Partly implemented.

`CanSim` is implemented as a host-side simulator. `CanDisplay` has the first hardware-independent decoder slice. ESP32-S3 TWAI integration, display rendering, and real bus validation are still planned.

## Context

The project models a small vehicle telemetry pipeline. The simulator emits fake car data in the same shape that a future ESP32-S3 receiver should consume.

Final vehicle path:

```text
Car -> Vehicle CAN bus -> ESP32-S3 CAN interface -> Display
```

Development path:

```text
CanSim -> console / vcan0 / USB-CAN adapter -> CanDisplay decoder -> future display renderer
```

The simulator does not directly call display code in the real hardware path. It emits CAN frames. The receiver listens to those frames as if they came from a vehicle.

## Project Boundaries

### CanSim

`CanSim` owns vehicle-state generation and CAN frame transmission.

Implemented responsibilities:

- Parse CLI options.
- Select static scenarios: `idle`, `cruise`, and `braking`.
- Apply manual telemetry overrides.
- Encode vehicle state into CAN frames.
- Load IDs, periods, and signal layout from `CanSim/config/can-signals.yaml`.
- Send frames to console output, recording test output, or Linux SocketCAN.
- Schedule messages by configured cycle time when `--duration` is used.

### CanDisplay

`CanDisplay` owns receiver-side decoding and, later, display rendering.

Implemented responsibilities:

- Represent a received CAN frame as `candisplay::CanFrameView`.
- Decode simulator CAN frames into `candisplay::TelemetryState`.
- Test decoder behavior on the host without ESP32 hardware.

Planned responsibilities:

- Convert ESP32-S3 TWAI messages into `CanFrameView`.
- Maintain latest telemetry state on device.
- Render telemetry state on the chosen display.
- Handle missing, stale, or invalid CAN data gracefully.

## Current CAN Contract

The signal map is versioned in `CanSim/config/can-signals.yaml`.

| CAN ID | Message | Signals | Period |
| --- | --- | --- | --- |
| `0x100` | `VehicleMotion` | `speed_kph`, `throttle_percent`, `brake_active` | 100 ms |
| `0x101` | `EngineStatus` | `engine_rpm`, `engine_temp_c` | 100 ms |
| `0x102` | `PowerStatus` | `fuel_percent`, `battery_voltage` | 500 ms |
| `0x103` | `GearStatus` | `selected_gear` | 100 ms |
| `0x700` | `Diagnostics` | `fault_active`, `active_dtc` | 1000 ms |

The current decoder in `CanDisplay` mirrors this contract manually. A later improvement can generate shared simulator and display constants from the same signal map.

## Simulator Runtime Shape

```text
CLI Options
      |
Scenario / Manual Values
      |
VehicleState
      |
SignalMapCanEncoder
      |
CanFrame list
      |
CanOutput
```

Output adapters:

- `ConsoleCanOutput`: local development and smoke tests.
- `RecordingCanOutput`: unit tests.
- `SocketCanOutput`: Linux `vcan0` or real SocketCAN interfaces.

## Receiver Runtime Shape

```text
CAN frame bytes
      |
CanFrameView
      |
decodeCanFrame()
      |
TelemetryState
      |
Display renderer
```

For ESP32-S3, the TWAI driver should be the only hardware-specific part of the receive path. The decoder should remain portable C++ where practical.

## SocketCAN Strategy

SocketCAN gives the project two useful test modes:

- `vcan0`: local Linux and CI testing without hardware.
- `can0`: real USB-CAN adapter testing on a bench bus.

Expected Linux virtual CAN flow:

```text
CanSim -> SocketCAN output -> vcan0 -> candump / integration checks
```

Expected hardware bench flow:

```text
CanSim -> SocketCAN can0 -> USB-CAN adapter -> CANH/CANL -> SN65HVD230 -> ESP32-S3
```

macOS can build and run the platform-independent pieces, but it cannot run SocketCAN output directly.

## CI/CD Strategy

CI should prove everything that does not require physical hardware:

- Configure CMake from the repository root.
- Build both `CanSim` and `CanDisplay`.
- Run simulator and display decoder tests.
- Run CLI smoke tests.
- Run formatting checks.
- On Linux, create `vcan0` and verify SocketCAN output.

Hardware-in-the-loop checks should be added later with a manual process or self-hosted runner.

## Design Decisions

### CAN remains the boundary

The display receiver should consume CAN frames, not simulator internals. That keeps the development path close to the final vehicle path.

### Keep the decoder host-testable

CAN decoding is core behavior and should remain testable on a laptop. ESP32-specific code should adapt hardware frames into portable frame types.

### Keep the signal map versioned

CAN IDs, periods, scale factors, and payload layout belong in version control. The simulator already consumes `CanSim/config/can-signals.yaml`; the display side should eventually share or generate from the same source of truth.

### CLI first, richer controls later

The CLI is the first control surface because it is simple, scriptable, and CI-friendly. A later button or UI surface should call the same simulator logic instead of duplicating scenario or encoding behavior.

## Open Questions

- Should `CanDisplay` generate decoder constants from `CanSim/config/can-signals.yaml`, or should both sides eventually consume a shared generated header?
- Which display module and graphics library will be used?
- Should fault injection affect only diagnostics, or also change normal telemetry values?
- Should hardware-in-the-loop tests run manually, or should a self-hosted runner own them later?

## Next Implementation Steps

1. Add `--print-state`, `--list-scenarios`, and `--list-messages` to `CanSim`.
2. Add time-varying scenarios to `CanSim`.
3. Add named fault injection.
4. Add an ESP32 TWAI adapter in `CanDisplay`.
5. Add a display renderer interface that consumes `TelemetryState`.
