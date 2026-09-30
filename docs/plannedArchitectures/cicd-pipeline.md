# CI/CD Pipeline Architecture

## Status

Planned.

## Context

The project is a C++ CAN telemetry simulator. The application itself matters, but the main learning goal is understanding and practicing CI/CD with a realistic C++ project.

CI/CD means:

- Continuous Integration: automatically prove that code still builds and behaves correctly whenever it changes.
- Continuous Delivery: automatically package code so it is ready to release.
- Continuous Deployment: automatically release or deploy code after it passes the required checks.

For this project, CI/CD should make the simulator trustworthy before it is used with virtual CAN, real CAN hardware, microcontrollers, the CLI control surface, or later button-based controls.

## Pipeline Goals

The pipeline should answer these questions on every change:

- Does the C++ code compile?
- Does it compile on more than one platform?
- Are the unit tests passing?
- Are CAN frame encoders producing the expected bytes?
- Does the simulator work with virtual CAN on Linux?
- Is the code formatted consistently?
- Do static analysis tools find obvious problems?
- Can we package the simulator as a runnable artifact?
- Can we produce a release when a version tag is created?

## Planned Pipeline Stages

### 1. Checkout

The pipeline downloads the repository source code into a clean runner.

This matters because the build must work on a fresh machine, not only on one developer laptop.

### 2. Configure

CMake configures the project and generates build files.

Planned command shape:

```text
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

### 3. Build

The pipeline compiles the simulator.

Planned command shape:

```text
cmake --build build --parallel
```

This catches compiler errors, missing files, missing dependencies, and broken CMake configuration.

### 4. Unit Tests

The pipeline runs fast tests that do not require real hardware.

Initial unit test targets:

- Vehicle state updates
- Driving scenario behavior
- CAN message encoding
- Signal scaling and bounds
- Diagnostic frame generation

Example checks:

- Speed `100.0 kph` encodes into the expected CAN payload bytes.
- Brake state toggles the expected bit.
- Engine temperature is clamped or encoded correctly.

### 5. Formatting

The pipeline checks source formatting.

Planned tool:

- `clang-format`

This keeps code style consistent and avoids formatting-only review noise.

### 6. Static Analysis

The pipeline runs tools that inspect code without executing it.

Planned tools:

- `clang-tidy`
- Compiler warnings treated seriously

This can catch issues such as unsafe conversions, unused values, suspicious copies, and incorrect control-flow usage.

### 7. Sanitizer Builds

The pipeline runs extra builds that catch runtime bugs.

Planned sanitizers:

- AddressSanitizer for invalid memory access
- UndefinedBehaviorSanitizer for undefined C++ behavior

This is especially useful in C++, where memory and lifetime bugs can hide behind passing tests.

### 8. Virtual CAN Integration Tests

Linux CI should create a `vcan0` interface and run tests against it.

Planned flow:

```text
create vcan0
start simulator
listen for CAN frames
verify expected CAN IDs and payloads
stop simulator
```

This proves the SocketCAN path works without requiring physical hardware in CI.

### 9. Package

The pipeline creates runnable artifacts.

Initial artifact:

- Simulator binary

Later artifacts:

- Docker image
- Example microcontroller firmware package
- Release archive with config files and documentation

### 10. Release

When a version tag is pushed, the pipeline should create a release.

Example tag:

```text
v0.1.0
```

Release outputs:

- Compiled binaries
- Checksums
- Example CAN signal map
- Usage documentation

## Branch Strategy

Initial branch strategy should stay simple:

- Feature branches for development
- Pull requests into `main`
- CI required before merge
- Version tags for releases

`main` should represent code that builds, passes tests, and is safe to package.

## First GitHub Actions Workflow

The first CI workflow should run on:

- Pull requests to `main`
- Pushes to `main`

Initial jobs:

- Linux build and test
- Formatting check
- Static analysis

After the first C++ skeleton exists, add:

- Linux `vcan` integration test
- macOS build
- Windows build
- Release workflow

## Project-Specific CI/CD Map

| Project Area | CI/CD Protection |
| --- | --- |
| Vehicle state model | Unit tests |
| CAN signal encoder | Byte-level unit tests |
| SocketCAN adapter | Linux `vcan` integration tests |
| CLI control surface | CLI smoke tests |
| Button control surface | Input mapping and behavior tests |
| Config files | Schema validation |
| Releases | Packaged binaries and checksums |
| Docker support | Image build and container smoke test |

## Decisions

### 1. CI comes before hardware dependency

The pipeline should prove as much as possible without hardware. Hardware testing can be added later as a manual or self-hosted runner job.

### 2. Linux is the first full-fidelity CI target

Linux gets the richest checks because SocketCAN and `vcan` are Linux-native.

### 3. macOS and Windows are compile-and-unit-test targets first

macOS and Windows should build the core simulator and run platform-independent tests. They do not need SocketCAN integration tests initially.

### 4. Releases are tag-driven

Release packaging should happen when a version tag is pushed, not on every commit.

## Open Questions

- Should the project use GitHub Actions, GitLab CI, or another CI provider?
- Should package publishing include Docker from the first milestone?
- Should hardware-in-the-loop CAN testing be added later using a self-hosted runner?
- Should the later button control surface be tested as part of the main simulator pipeline or with a separate hardware/UI workflow?

## First Implementation Milestone

Create a minimal pipeline that:

- Configures the CMake project
- Builds the C++ simulator
- Runs unit tests
- Checks formatting

Then expand it with:

- `clang-tidy`
- Sanitizers
- `vcan` integration tests
- Release artifacts
