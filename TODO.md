# Project TODOs

Last updated: 2026-10-01

## Immediate

- [ ] Add `--print-state` to print the resolved `VehicleState` after applying defaults, scenarios, and manual CLI overrides.
- [ ] Improve `--help` text with short descriptions for timing, output, and signal-map options.
- [ ] Add `--list-scenarios` for discoverability.
- [ ] Add `--list-messages` to print CAN IDs, message names, and periods from `config/can-signals.yaml`.

## Scenario Runtime

- [ ] Add time-varying scenario updates instead of static scenario state.
- [ ] Implement an acceleration ramp scenario.
- [ ] Implement a braking event scenario.
- [ ] Add slow-changing telemetry behavior such as temperature rise, battery drift, or fuel drain.

## Fault Injection

- [ ] Add named faults such as `engine_overheat`, `low_battery`, and `sensor_fault`.
- [ ] Add a direct DTC fault form such as `--fault dtc:0x0420`.
- [ ] Make faults update both regular telemetry and diagnostics frames consistently.
- [ ] Add tests for fault-to-frame encoding behavior.

## SocketCAN And VM Workflow

- [ ] Add a helper script such as `scripts/setup-vcan.sh`.
- [ ] Add a helper script for SocketCAN smoke testing with `candump`.
- [ ] Document the Mac-to-VM sync and rebuild workflow in one place.
- [ ] Add CI or manual checks that verify scheduled SocketCAN output IDs and timing shape.

## Hardware Validation

- [ ] Test simulator output through a Linux `vcan0` interface.
- [ ] Test simulator output through a USB-CAN adapter.
- [ ] Test the microcontroller receiver path.
- [ ] Validate display behavior from simulated vehicle states and faults.

## CI/CD Polish

- [ ] Add release build artifacts.
- [ ] Add static analysis.
- [ ] Add sanitizer builds.
- [ ] Add packaged binaries or install targets.

## Control Surface

- [ ] Add a button or UI control surface after the simulator runtime is stable.
- [ ] Keep the UI calling the same simulator logic as the CLI.
- [ ] Avoid duplicating scenario, state, encoding, or output behavior in the UI layer.
