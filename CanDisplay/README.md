# CanDisplay

`CanDisplay` is the beginning of the ESP32-S3 display receiver codebase.

The first slice is hardware-independent C++ so it can be built and tested before the ESP32-S3 and display arrive.

## What Lives Here

| Path | Purpose |
| --- | --- |
| `include/candisplay/` | Display-side frame and telemetry types. |
| `src/can_decoder.cpp` | Decoder for the simulator's CAN frames. |
| `tests/can_decoder_test.cpp` | Host-side decoder tests. |
| `CMakeLists.txt` | Display-side library, test target, warnings, and format check. |

## Current API

- `candisplay::CanFrameView` describes a received standard CAN frame.
- `candisplay::TelemetryState` holds the latest decoded dashboard values.
- `candisplay::decodeCanFrame()` decodes the simulator's current CAN IDs.

The decoder currently mirrors `../CanSim/config/can-signals.yaml`.

## Intended ESP32 Flow

```text
ESP32-S3 TWAI receive
      |
CanFrameView
      |
decodeCanFrame()
      |
TelemetryState
      |
display renderer
```

ESP32-specific code should stay thin: receive TWAI frames, convert them to `CanFrameView`, update `TelemetryState`, then render the state on the display.

## Build And Test

From the repository root:

```bash
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build -R candisplay --output-on-failure
```

## Next Steps

- Add an ESP32 adapter that maps TWAI frames to `CanFrameView`.
- Add a display model or renderer interface that consumes `TelemetryState`.
- Once the exact display is known, add the matching display driver integration.
