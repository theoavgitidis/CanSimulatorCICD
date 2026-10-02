# Project Handoff

Last updated: 2026-10-02

## Project Summary

This project contains a C++ CAN telemetry simulator and the start of an ESP32-S3 display receiver codebase, with CI/CD practice built around both.

Target real-world path:

```text
Car -> Vehicle CAN bus -> ESP32-S3 CAN interface -> Display
```

Development/test path:

```text
CanSim -> console, vcan0, or USB-CAN adapter -> CanDisplay / ESP32-S3
```

The simulator currently uses a CLI. A button-based control surface may come later, but it should call the same simulator logic instead of duplicating it.

## Current State

Implemented:

- Root CMake-based C++20 project with `CanSim` and `CanDisplay` subprojects
- `CanFrame` and `VehicleState` types
- CAN frame encoding for IDs `0x100`, `0x101`, `0x102`, `0x103`, and `0x700`
- CLI scenarios: `idle`, `cruise`, `braking`
- CLI manual overrides for speed, throttle, brake, RPM, temperature, fuel, battery, gear, DTC, frame count, output mode, and CAN interface
- Console output backend
- Recording output backend for tests
- Linux SocketCAN output backend
- Output factory
- CAN signal map file at `CanSim/config/can-signals.yaml`
- YAML-driven signal packing and message periods loaded from `CanSim/config/can-signals.yaml`
- Hardcoded encoder retained as a compatibility baseline in tests
- Timed repeated sending with per-message periods loaded from `CanSim/config/can-signals.yaml`
- Fixed-period timing override with `--period-ms`
- Testable `CanScheduler` module
- Host-testable `CanDisplay` CAN decoder and telemetry state
- Unit tests, display decoder tests, formatting checks, and CLI smoke tests through CTest
- GitHub Actions CI with Linux `vcan0` SocketCAN smoke test
- README and architecture docs

Not implemented yet:

- Fault names such as `engine_overheat`
- `--print-state`
- ESP32-S3 TWAI receive adapter
- Display renderer implementation
- Real hardware USB-CAN validation
- Button-based control surface

## Important Files

- `CMakeLists.txt`: root build, tests, and aggregate formatting target
- `CanSim/CMakeLists.txt`: simulator build, tests, and formatting check
- `CanSim/src/main.cpp`: CLI executable entry point
- `CanSim/include/cansim/cli_options.hpp` and `CanSim/src/cli_options.cpp`: CLI parsing
- `CanSim/include/cansim/can_encoder.hpp` and `CanSim/src/can_encoder.cpp`: hardcoded baseline and YAML-driven CAN frame encoding
- `CanSim/include/cansim/can_scheduler.hpp` and `CanSim/src/can_scheduler.cpp`: per-message timing
- `CanSim/include/cansim/can_signal_map.hpp` and `CanSim/src/can_signal_map.cpp`: signal packing and schedule loading from `CanSim/config/can-signals.yaml`
- `CanSim/include/cansim/can_output.hpp`: output interface
- `CanSim/src/console_can_output.cpp`: terminal output backend
- `CanSim/src/socket_can_output_linux.cpp`: Linux SocketCAN backend
- `CanSim/src/can_output_factory.cpp`: creates selected output backend
- `CanSim/tests/`: simulator unit and smoke tests
- `CanDisplay/include/candisplay/`: display-side frame and telemetry types
- `CanDisplay/src/can_decoder.cpp`: display-side decoder for simulator CAN frames
- `CanDisplay/tests/`: display-side unit tests
- `CanSim/README.md` and `CanDisplay/README.md`: subproject documentation
- `.github/workflows/ci.yml`: GitHub Actions CI
- `docs/plannedArchitectures/`: architecture notes and diagrams
- `docs/hardware/`: ESP32-S3 and CAN transceiver notes

## Local Mac Workflow

Build:

```bash
cmake -S . -B build
cmake --build build --parallel
```

Run tests:

```bash
ctest --test-dir build --output-on-failure
```

Run console output:

```bash
./build/cansim --scenario cruise --frames 1 --output console
```

Run scheduled timed output from the repository root:

```bash
./build/cansim --scenario cruise --duration 10 --output console
```

Expected macOS SocketCAN behavior:

```bash
./build/cansim --scenario cruise --frames 1 --output socketcan --interface vcan0
```

Expected result:

```text
Selected CAN output is not supported on this platform
```

This is correct because SocketCAN is Linux-only.

## Linux VM Workflow

If copying from Mac without GitHub:

```bash
rsync -av --exclude build /Users/theoa/Desktop/Projects/CanSimulatorCICD/ theoa@192.168.64.2:/home/theoa/CanSimulatorCICD/
```

On the VM:

```bash
cd ~/CanSimulatorCICD
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

If the VM shows fewer CTest entries than expected, that is usually because `clang-format` was not installed when CMake configured the build. The core simulator and display tests can still pass, but formatting checks will be disabled until CMake is reconfigured with `clang-format` installed.

## SocketCAN / vcan0 Manual Test

In the Linux VM, set up `vcan0`:

```bash
sudo modprobe can
sudo modprobe can_raw
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
ip -details link show vcan0
```

If `vcan0` already exists, `ip link add` may report:

```text
RTNETLINK answers: File exists
```

That is okay. Continue with `sudo ip link set up vcan0`.

Use two terminals connected to the same VM.

Terminal 1:

```bash
candump vcan0
```

Terminal 2:

```bash
cd ~/CanSimulatorCICD
./build/cansim --scenario cruise --frames 1 --output socketcan --interface vcan0
```

Expected `candump` output:

```text
vcan0  100   [8] ...
vcan0  101   [8] ...
vcan0  102   [8] ...
vcan0  103   [8] ...
vcan0  700   [8] ...
```

Important: `vcan0` is local to one Linux kernel. If two separate VMs are open, `candump` in one VM will not see CAN frames sent in the other VM.

## SSH Into VM

The known VM IP during testing was:

```text
192.168.64.2
```

From macOS:

```bash
ssh theoa@192.168.64.2
```

If SSH is refused, install/start SSH inside the VM:

```bash
sudo apt install -y openssh-server
sudo systemctl enable --now ssh
sudo systemctl status ssh --no-pager
```

## GitHub Actions CI

The CI currently:

- Checks out the repository
- Installs `can-utils`, `clang-format`, and matching Linux extra kernel modules
- Configures CMake
- Builds
- Runs CTest
- Runs formatting checks for both subprojects
- Runs CLI console smoke test
- Sets up `vcan0`
- Runs SocketCAN smoke test with `candump`
- Verifies CAN IDs `100`, `101`, `102`, `103`, and `700`

The `vcan` module required this package in CI:

```bash
linux-modules-extra-$(uname -r)
```

Without it, CI failed with:

```text
modprobe: FATAL: Module vcan not found
```

## Known Notes

- macOS cannot run the SocketCAN backend.
- Each VM needs its own `vcan0` setup.
- `candump vcan0` waits silently until frames arrive.
- The simulator first looks for `config/can-signals.yaml` relative to the current working directory, then falls back to `CanSim/config/can-signals.yaml` when run from the repository root. Use `--signal-map PATH` for any other map.

## Recommended Next Implementation Step

Add `--print-state` so scenario defaults and manual CLI overrides can be inspected before frames
are encoded or sent.

This should print the resolved `VehicleState` and exit without requiring CAN output setup.

After that, add the ESP32-S3 TWAI adapter in `CanDisplay` once the board is available.

## Quick Resume Commands

Mac:

```bash
cd /Users/theoa/Desktop/Projects/CanSimulatorCICD
git status
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Linux VM:

```bash
cd ~/CanSimulatorCICD
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

SocketCAN smoke in one Linux VM:

```bash
candump -n 5 vcan0 > candump.log &
CANDUMP_PID=$!
sleep 1
./build/cansim --scenario cruise --frames 1 --output socketcan --interface vcan0
wait "$CANDUMP_PID"
cat candump.log
```
