# Architecture Diagrams

## Status

Planned.

## System Diagram

```mermaid
flowchart LR
    user["Developer / Tester"]
    cli["CLI Control"]
    buttons["Button Control Surface<br/>planned later"]
    control["Simulator Control Interface"]

    subgraph simulator["C++ Simulator Runtime"]
        scenario["Scenario Engine"]
        state["Vehicle State Model"]
        faults["Fault Injection"]
        scheduler["Message Scheduler"]
        encoder["CAN Signal Encoder"]
        app["Simulator Orchestrator"]
    end

    signalMap["Versioned CAN Signal Map<br/>YAML first, DBC later"]

    subgraph outputs["CAN Output Adapters"]
        console["Console Output"]
        socketcan["SocketCAN Output"]
        recording["Recording Test Output"]
    end

    subgraph canTargets["CAN Targets"]
        vcan["vcan0<br/>Local and CI"]
        can0["can0<br/>USB-CAN Hardware"]
        mcu["Microcontroller<br/>ESP32 / STM32 / MCP2515"]
        tools["CAN Tools / Integration Tests"]
    end

    user --> cli
    user --> buttons
    cli --> control
    buttons --> control
    control <--> app

    app --> scenario
    app --> faults
    app --> scheduler
    scenario --> state
    faults --> state
    state --> encoder
    signalMap --> encoder
    scheduler --> encoder

    encoder --> console
    encoder --> socketcan
    encoder --> recording

    socketcan --> vcan
    socketcan --> can0
    vcan --> tools
    can0 --> mcu
    recording --> tools
```

## Target Hardware Diagram

```mermaid
flowchart LR
    car["Car"]
    vehicleCan["Vehicle CAN Bus"]
    transceiver["CAN Transceiver"]
    microcontroller["Microcontroller<br/>CAN Controller"]
    display["Display"]

    car --> vehicleCan
    vehicleCan --> transceiver
    transceiver --> microcontroller
    microcontroller --> display
```

## Simulator Development Path

```mermaid
flowchart LR
    simulator["C++ CAN Simulator"]
    socketcan["SocketCAN Output"]
    adapter["vcan0 or USB-CAN Adapter"]
    transceiver["CAN Transceiver<br/>for hardware path"]
    microcontroller["Microcontroller<br/>CAN Controller"]
    display["Display"]

    simulator --> socketcan
    socketcan --> adapter
    adapter --> transceiver
    transceiver --> microcontroller
    microcontroller --> display
```

## Class Diagram

```mermaid
classDiagram
    class Simulator {
        +start()
        +stop()
        +tick(delta_ms)
        +setScenario(name)
        +injectFault(code)
    }

    class VehicleState {
        +double speed_kph
        +int engine_rpm
        +double engine_temp_c
        +double fuel_percent
        +double battery_voltage
        +double throttle_percent
        +bool brake_active
        +int gear
        +string active_dtc
        +clamp()
    }

    class ScenarioEngine {
        +loadScenario(name)
        +update(VehicleState, delta_ms)
    }

    class FaultInjector {
        +activate(code)
        +clear()
        +apply(VehicleState)
    }

    class CanSignalMap {
        +load(path)
        +messageByName(name)
        +messageById(id)
    }

    class CanFrame {
        +uint32_t id
        +uint8_t[8] data
        +uint8_t dlc
    }

    class CanEncoder {
        +encodeMotion(VehicleState) CanFrame
        +encodeEngine(VehicleState) CanFrame
        +encodePower(VehicleState) CanFrame
        +encodeGear(VehicleState) CanFrame
        +encodeDiagnostics(VehicleState) CanFrame
        +encodeAll(VehicleState) vector~CanFrame~
    }

    class MessageScheduler {
        +framesDue(now_ms) vector~MessageName~
        +markSent(message, now_ms)
    }

    class CanOutput {
        <<interface>>
        +open()
        +send(CanFrame)
        +close()
    }

    class ConsoleCanOutput {
        +send(CanFrame)
    }

    class SocketCanOutput {
        +open(interface_name)
        +send(CanFrame)
        +close()
    }

    class RecordingCanOutput {
        +send(CanFrame)
        +frames() vector~CanFrame~
    }

    class SimulatorControl {
        +startSimulation()
        +stopSimulation()
        +setScenario(name)
        +setManualValue(signal, value)
        +injectFault(code)
        +currentState() VehicleState
    }

    class CliController {
        +parseArguments(args)
        +runCommand()
        +printState()
        +printFrames()
    }

    class ButtonController {
        +mapButton(input)
        +handlePress(input)
    }

    Simulator o-- VehicleState
    Simulator --> ScenarioEngine
    Simulator --> FaultInjector
    Simulator --> MessageScheduler
    Simulator --> CanEncoder
    Simulator --> CanOutput

    ScenarioEngine --> VehicleState
    FaultInjector --> VehicleState
    CanEncoder --> VehicleState
    CanEncoder --> CanSignalMap
    CanEncoder --> CanFrame
    MessageScheduler --> CanSignalMap

    CanOutput <|.. ConsoleCanOutput
    CanOutput <|.. SocketCanOutput
    CanOutput <|.. RecordingCanOutput

    SimulatorControl --> Simulator
    CliController --> SimulatorControl
    ButtonController --> SimulatorControl
```

## CI/CD Implementation Diagram

```mermaid
flowchart TD
    change["Code Change"]
    pr["Pull Request"]
    main["Push to main"]
    tag["Version Tag<br/>v0.1.0"]

    checkout["Checkout Repository"]
    tools["Install Build Tools<br/>CMake, compiler, clang-format"]
    configure["Configure<br/>cmake -S . -B build"]
    build["Build<br/>cmake --build build --parallel"]
    unit["Run Unit Tests<br/>ctest --test-dir build"]
    cliSmoke["Run CLI Smoke Tests<br/>start, scenario, frames"]
    format["Formatting Check<br/>clang-format"]
    static["Static Analysis<br/>clang-tidy"]
    sanitizer["Sanitizer Build<br/>ASan / UBSan"]

    linuxOnly["Linux SocketCAN Job"]
    vcanSetup["Create vcan0"]
    integration["Run vcan Integration Tests"]

    package["Package Simulator Binary"]
    artifacts["Upload CI Artifacts"]
    release["Create Release"]
    checksums["Generate Checksums"]

    change --> pr
    change --> main
    change --> tag

    pr --> checkout
    main --> checkout
    tag --> checkout

    checkout --> tools
    tools --> configure
    configure --> build
    build --> unit
    build --> cliSmoke
    build --> format
    build --> static
    unit --> sanitizer
    cliSmoke --> sanitizer

    unit --> linuxOnly
    cliSmoke --> linuxOnly
    linuxOnly --> vcanSetup
    vcanSetup --> integration

    unit --> package
    integration --> package
    package --> artifacts

    tag --> release
    artifacts --> release
    release --> checksums
```

## CI/CD Timing Recommendation

CI/CD should begin as soon as the first compileable function and its first test exist.

The first pipeline should be intentionally small:

- configure the CMake project
- build the simulator library or executable
- run the first unit test
- optionally check formatting

The reason is practical: CI/CD is easiest when it grows with the project. If it is added after many features already exist, the team has to debug the application and the pipeline at the same time. Starting early makes every later feature pass through the same build and test path from the beginning.

Recommended order:

1. Create the project skeleton.
2. Add the first pure function, such as CAN speed encoding.
3. Add a unit test for that function.
4. Add the first CI workflow that builds and runs the test.
5. Expand CI as the architecture expands.

Do not wait for SocketCAN, the button control surface, packaging, or hardware support before adding CI. Those should be added as later CI stages once the matching project capability exists.
