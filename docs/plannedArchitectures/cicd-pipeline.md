# CI/CD Pipeline Architecture

Last updated: 2026-10-02

## Status

Partly implemented.

The current GitHub Actions workflow builds and tests both CMake subprojects, runs formatting checks, runs a CLI smoke test, and verifies SocketCAN output on Linux `vcan0`.

## Context

The project has two C++ areas:

- `CanSim`: simulator, CAN encoder, CLI, and output adapters.
- `CanDisplay`: display-side CAN decoder and future ESP32 receiver code.

CI/CD should keep both areas trustworthy before real hardware is available. The pipeline should prove host-buildable behavior first, then add hardware checks later.

## Current Workflow

The workflow at `.github/workflows/ci.yml` runs on:

- Pushes to `main`.
- Pull requests to `main`.
- Manual `workflow_dispatch`.

Current Linux job:

1. Check out the repository.
2. Show CMake and compiler versions.
3. Install `can-utils`, `clang-format`, and Linux extra kernel modules.
4. Configure CMake from the repository root.
5. Build all targets.
6. Run all CTest tests.
7. Run all formatting checks matching `format_check`.
8. Run a console CLI smoke test.
9. Configure `vcan0`.
10. Run a SocketCAN smoke test and verify CAN IDs `100`, `101`, `102`, `103`, and `700`.

## Local CI Equivalent

Most checks can be run locally from the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
ctest --test-dir build -R "format_check" --output-on-failure
./build/cansim --scenario cruise --frames 1 --output console
```

SocketCAN checks require Linux:

```bash
sudo modprobe can
sudo modprobe can_raw
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
```

Then:

```bash
candump -n 5 vcan0 > candump.log &
CANDUMP_PID=$!
sleep 1
sudo ./build/cansim --scenario cruise --frames 2 --output socketcan --interface vcan0
wait "$CANDUMP_PID"
cat candump.log
```

## Pipeline Goals

The pipeline should answer these questions on every change:

- Does the root CMake project configure from a clean checkout?
- Do both `CanSim` and `CanDisplay` compile?
- Are simulator unit and smoke tests passing?
- Are display decoder tests passing?
- Are CAN encoders producing expected byte payloads?
- Does the simulator still work with Linux virtual CAN?
- Are C++ files formatted consistently?
- Can release artifacts be produced later without changing project layout?

## Current Test Coverage

Current CTest coverage includes:

- CAN encoder compatibility.
- CLI option parsing.
- Scenario parsing and defaults.
- Output adapters.
- Output factory behavior.
- Scheduled message timing.
- Signal map parsing.
- CLI smoke behavior.
- Display-side CAN frame decoding.
- Simulator and display formatting checks when `clang-format` is available.

## Planned Pipeline Stages

### Static Analysis

Add `clang-tidy` once the compile commands and target layout are stable.

Recommended scope:

- `CanSim/src`
- `CanSim/include`
- `CanDisplay/src`
- `CanDisplay/include`

### Sanitizers

Add AddressSanitizer and UndefinedBehaviorSanitizer builds for host-side code.

These should run on simulator tests and display decoder tests.

### Multi-Platform Builds

Linux remains the full-fidelity target because SocketCAN is Linux-native.

macOS and Windows should eventually build and test:

- `cansim_core`
- `candisplay_core`
- platform-independent unit tests

SocketCAN tests should stay Linux-only.

### Packaging

Initial release artifacts should include:

- `cansim` executable.
- `CanSim/config/can-signals.yaml`.
- README and hardware documentation.

Later artifacts can include ESP32 firmware packages once `CanDisplay` grows device-specific code.

### Hardware-In-The-Loop

Bench hardware checks should be added only after the USB-CAN adapter, ESP32-S3 board, transceiver, and display are validated manually.

Possible later flow:

```text
CanSim -> USB-CAN adapter -> CAN bus -> ESP32-S3 -> serial/display assertion
```

This should probably run as a manual job or on a self-hosted runner.

## Decisions

### CI comes before hardware dependency

The pipeline should prove as much as possible without physical hardware.

### Root CMake is the entry point

CI should continue to configure from the repository root with `cmake -S . -B build`. Subprojects can still be reasoned about independently, but the integrated repo must always build as one workspace.

### Linux owns SocketCAN coverage

SocketCAN and `vcan` are Linux kernel features, so CI should keep those checks in a Linux job.

### Formatting is checked through CTest

Both subprojects register `*_format_check` tests. The CI workflow runs them with:

```bash
ctest --test-dir build -R "format_check" --output-on-failure
```

## Open Questions

- Should static analysis warnings become blocking immediately or start as informational?
- Should release packaging use CPack, a custom archive step, or both?
- Should hardware-in-the-loop checks use a self-hosted runner?
- Should generated code from the CAN signal map be part of CI once shared headers are introduced?

## Next CI/CD Steps

1. Add `clang-tidy`.
2. Add sanitizer builds.
3. Add macOS and Windows host-build jobs.
4. Add release packaging for `cansim` plus `CanSim/config/can-signals.yaml`.
5. Add hardware-in-the-loop only after manual bench validation.
