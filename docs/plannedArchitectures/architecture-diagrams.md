# Architecture Diagrams

Last updated: 2026-10-02

## Status

Partly implemented.

These diagrams show the current repo shape and the intended hardware path. `CanSim` and the portable `CanDisplay` decoder exist today. ESP32 TWAI glue and display rendering are planned next.

## Repository Diagram

```mermaid
flowchart TB
    repo["CanSimulatorCICD"]

    subgraph canSim["CanSim"]
        simCli["CLI"]
        simState["VehicleState"]
        simScenario["Scenarios"]
        simMap["can-signals.yaml"]
        simEncoder["CAN Encoder"]
        simScheduler["Message Scheduler"]
        simOutput["CAN Output Adapters"]
        simTests["Simulator Tests"]
    end

    subgraph canDisplay["CanDisplay"]
        frameView["CanFrameView"]
        decoder["decodeCanFrame()"]
        telemetry["TelemetryState"]
        displayTests["Decoder Tests"]
        esp32Future["Future ESP32 TWAI Adapter"]
        rendererFuture["Future Display Renderer"]
    end

    docs["docs/"]
    ci["GitHub Actions CI"]

    repo --> canSim
    repo --> canDisplay
    repo --> docs
    repo --> ci

    simCli --> simScenario
    simScenario --> simState
    simState --> simEncoder
    simMap --> simEncoder
    simMap --> simScheduler
    simEncoder --> simOutput
    simScheduler --> simOutput
    simTests --> simEncoder
    simTests --> simOutput

    frameView --> decoder
    decoder --> telemetry
    displayTests --> decoder
    esp32Future --> frameView
    telemetry --> rendererFuture

    simMap -. "shared CAN contract" .-> decoder
```

## Development Data Flow

```mermaid
flowchart LR
    cli["Developer / CI"]
    cansim["CanSim"]
    console["Console Output"]
    vcan["Linux vcan0"]
    usbCan["USB-CAN Adapter"]
    candump["candump / tests"]
    canBus["Bench CAN Bus"]
    transceiver["SN65HVD230"]
    esp32["ESP32-S3 TWAI"]
    candisplay["CanDisplay Decoder"]
    display["Display Renderer"]

    cli --> cansim
    cansim --> console
    cansim --> vcan
    vcan --> candump
    cansim --> usbCan
    usbCan --> canBus
    canBus --> transceiver
    transceiver --> esp32
    esp32 --> candisplay
    candisplay --> display
```

## Final Hardware Path

```mermaid
flowchart LR
    car["Car"]
    vehicleCan["Vehicle CAN Bus"]
    protection["Protection / Interface Hardware"]
    transceiver["CAN Transceiver"]
    esp32["ESP32-S3 TWAI Controller"]
    decoder["CanDisplay Decoder"]
    display["Display"]

    car --> vehicleCan
    vehicleCan --> protection
    protection --> transceiver
    transceiver --> esp32
    esp32 --> decoder
    decoder --> display
```

## Simulator Component Diagram

```mermaid
classDiagram
    class CliOptions {
        +double speed_kph
        +double throttle_percent
        +bool brake_active
        +int engine_rpm
        +double engine_temp_c
        +double fuel_percent
        +double battery_voltage
        +int gear
        +uint16_t active_dtc
        +int frames
        +optional~double~ duration_seconds
        +optional~int~ period_ms
        +OutputMode output_mode
        +string interface_name
        +string signal_map_path
    }

    class VehicleState {
        +double speed_kph
        +double throttle_percent
        +bool brake_active
        +int engine_rpm
        +double engine_temp_c
        +double fuel_percent
        +double battery_voltage
        +int gear
        +uint16_t active_dtc
    }

    class CanFrame {
        +uint32_t id
        +uint8_t dlc
        +array~uint8_t, 8~ data
    }

    class CanSignalMap {
        +messages
    }

    class SignalMapCanEncoder {
        +encodeAll(VehicleState) vector~CanFrame~
    }

    class CanScheduler {
        +dueMessageIds(elapsed) vector~uint32_t~
        +timeUntilNextDue(elapsed) milliseconds
    }

    class CanOutput {
        <<interface>>
        +send(CanFrame)
    }

    class ConsoleCanOutput
    class SocketCanOutput
    class RecordingCanOutput

    CliOptions --> VehicleState
    CanSignalMap --> SignalMapCanEncoder
    VehicleState --> SignalMapCanEncoder
    SignalMapCanEncoder --> CanFrame
    CanSignalMap --> CanScheduler
    CanFrame --> CanOutput
    CanOutput <|.. ConsoleCanOutput
    CanOutput <|.. SocketCanOutput
    CanOutput <|.. RecordingCanOutput
```

## Display Decoder Diagram

```mermaid
classDiagram
    class CanFrameView {
        +uint32_t id
        +uint8_t dlc
        +array~uint8_t, 8~ data
    }

    class TelemetryState {
        +double speed_kph
        +int throttle_percent
        +bool brake_active
        +int engine_rpm
        +int engine_temp_c
        +int fuel_percent
        +double battery_voltage
        +int gear
        +bool fault_active
        +uint16_t active_dtc
    }

    class CanDecoder {
        +decodeCanFrame(CanFrameView, TelemetryState) bool
    }

    class TwaiAdapter {
        <<planned>>
        +receive()
        +toCanFrameView()
    }

    class DisplayRenderer {
        <<planned>>
        +render(TelemetryState)
    }

    TwaiAdapter --> CanFrameView
    CanFrameView --> CanDecoder
    CanDecoder --> TelemetryState
    TelemetryState --> DisplayRenderer
```

## CI/CD Diagram

```mermaid
flowchart TD
    change["Code Change"]
    checkout["Checkout Repository"]
    tools["Install Tools"]
    configure["cmake -S . -B build"]
    build["cmake --build build --parallel"]
    tests["ctest --test-dir build"]
    format["ctest -R format_check"]
    cliSmoke["./build/cansim smoke"]
    linuxOnly["Linux Only"]
    vcanSetup["Create vcan0"]
    socketSmoke["SocketCAN smoke test"]
    future["Future: clang-tidy, sanitizers, releases"]

    change --> checkout
    checkout --> tools
    tools --> configure
    configure --> build
    build --> tests
    build --> cliSmoke
    tests --> format
    cliSmoke --> linuxOnly
    linuxOnly --> vcanSetup
    vcanSetup --> socketSmoke
    format --> future
    socketSmoke --> future
```
