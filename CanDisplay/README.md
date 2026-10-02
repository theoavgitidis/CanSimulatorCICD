# CanDisplay

ESP32-S3 display receiver code will live here.

The first slice is intentionally hardware-independent C++:

- `candisplay::CanFrameView` describes a received standard CAN frame.
- `candisplay::TelemetryState` holds the latest decoded dashboard values.
- `candisplay::decodeCanFrame()` decodes the simulator's current CAN IDs.

Later ESP32-specific code should stay thin: receive TWAI frames, convert them to
`CanFrameView`, update `TelemetryState`, then render the state on the display.

The decoder currently matches `../CanSim/config/can-signals.yaml`.
