# Project Handoff

Last updated: 2026-09-30

## Project Summary

This project is a C++ CAN telemetry simulator with CI/CD practice built around it.

Target real-world path:

```text
Car -> Vehicle CAN bus -> Microcontroller CAN interface -> Display
```

Development/test path:

```text
C++ CAN Simulator -> SocketCAN vcan0 or USB-CAN adapter -> CAN receiver tools / microcontroller
```

The simulator currently uses a CLI. A button-based control surface may come later, but it should call the same simulator logic instead of duplicating it.

## Current State

Implemented:

- CMake-based C++20 project
- `CanFrame` and `VehicleState` types
- CAN frame encoding for IDs `0x100`, `0x101`, `0x102`, `0x103`, and `0x700`
- CLI scenarios: `idle`, `cruise`, `braking`
- CLI manual overrides for speed, throttle, brake, RPM, temperature, fuel, battery, gear, DTC, frame count, output mode, and CAN interface
- Console output backend
- Recording output backend for tests
- Linux SocketCAN output backend
- Output factory
- CAN signal map file at `config/can-signals.yaml`
- Timed repeated sending with `--duration` and `--period-ms`
- Unit tests and CLI smoke test through CTest
- GitHub Actions CI with Linux `vcan0` SocketCAN smoke test
- README and architecture docs

Not implemented yet:

- Runtime scheduler for different CAN message periods
- Loading `config/can-signals.yaml` into the encoder
- Fault names such as `engine_overheat`
- `--print-state`
- Real hardware USB-CAN validation
- Button-based control surface

## Important Files

- `CMakeLists.txt`: build, tests, formatting target
- `src/main.cpp`: CLI executable entry point
- `include/cansim/cli_options.hpp` and `src/cli_options.cpp`: CLI parsing
- `include/cansim/can_encoder.hpp` and `src/can_encoder.cpp`: CAN frame encoding
- `include/cansim/can_output.hpp`: output interface
- `src/console_can_output.cpp`: terminal output backend
- `src/socket_can_output_linux.cpp`: Linux SocketCAN backend
- `src/can_output_factory.cpp`: creates selected output backend
- `tests/`: unit and smoke tests
- `.github/workflows/ci.yml`: GitHub Actions CI
- `docs/plannedArchitectures/`: architecture notes and diagrams

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

If the VM only shows 6 CTest tests, that is usually because `clang-format` was not installed when CMake configured the build. The core simulator tests can still pass.

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
- Runs formatting check
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

- Linux builds may show a warning about `expectFalse` being unused in `tests/can_output_factory_test.cpp`. This happens because that helper is only used on non-Linux builds. It does not currently fail the build.
- macOS cannot run the SocketCAN backend.
- Each VM needs its own `vcan0` setup.
- `candump vcan0` waits silently until frames arrive.

## Recommended Next Implementation Step

Add a runtime scheduler that sends each CAN message on its own period instead of sending every
message in every timed burst.

- Use per-message periods from the signal map:
  - `0x100`, `0x101`, `0x103`: 100 ms
  - `0x102`: 500 ms
  - `0x700`: 1000 ms

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
