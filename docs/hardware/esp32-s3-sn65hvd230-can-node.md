# ESP32-S3 SN65HVD230 CAN Receiver Node

Last updated: 2026-09-30

## Status

Selected hardware target, pending bench validation.

This page documents the microcontroller side of the project: the hardware that will listen to CAN frames produced by the simulator, then later drive the display or button/control firmware.

```text
C++ CAN Simulator
      |
Linux SocketCAN can0
      |
USB-CAN adapter
      |
CANH / CANL
      |
SN65HVD230 CAN transceiver module
      |
ESP32-S3 TWAI CAN controller
      |
Display / receiver firmware
```

The same receiver-side hardware can later listen to a real vehicle CAN bus instead of the simulator, but bench validation should happen first.

## Selected Hardware

| Part | Role | Notes |
| --- | --- | --- |
| ESP32-S3 dual Type-C development board module, `N16R8` / `N8R8`, with 44-pin expansion board | Microcontroller and CAN controller | ESP32-S3 has one TWAI controller. The exact flash/PSRAM variant should be checked on the module label when the board arrives. |
| SN65HVD230 CAN bus transceiver module | Physical CAN transceiver | 3.3 V CAN transceiver module for ESP32-level logic. Connects the ESP32-S3 TWAI TX/RX pins to the differential CANH/CANL bus. |
| USB-CAN adapter | Simulator-to-real-bus bridge | Still required on the computer side when the C++ simulator sends frames to physical CAN through SocketCAN `can0`. Not selected in this note. |

Important distinction: the ESP32-S3 provides the CAN controller through TWAI, but it does not include the CAN transceiver. The SN65HVD230 module is the physical transceiver, but it is not a USB-CAN adapter and it is not a standalone CAN controller.

## Marketplace References

Use marketplace pages as order references only. For electrical decisions, prefer the vendor datasheets and framework documentation in the sources section.

- ESP32-S3 board kit: <https://www.aliexpress.com/item/1005013281224381.html>
- User-provided SN65HVD230 order link: <https://www.ebay.de/itm/176794513601>

Note: on 2026-09-30, the eBay item URL above resolved publicly as an ILI9488 TFT display page, while the selected part description is an SN65HVD230 CAN transceiver module. Keep the selected component as "SN65HVD230 CAN bus transceiver module" and recheck the order page before relying on the marketplace link.

## Electrical Facts

- ESP32-S3 TWAI supports Classical CAN / CAN 2.0 style frames through an external transceiver. It is not a CAN FD controller.
- ESP32-S3 TWAI TX and RX can be routed to GPIO pads through the ESP32-S3 GPIO matrix.
- SN65HVD230 is a 3.3 V CAN transceiver family. Its logic-side pins are `D` / `TXD` for transmit input and `R` / `RXD` for receive output.
- SN65HVD230 bus-side pins are `CANH` and `CANL`.
- Some breakout boards label the logic pins as `CTX` and `CRX`. Treat those as controller-facing TX and RX:
  - ESP32 TWAI TX -> `CTX` / `TXD` / `D`
  - ESP32 TWAI RX <- `CRX` / `RXD` / `R`
- All CAN nodes on the physical bus must use the same bitrate.
- The transceiver module is suitable for a short bench CAN bus, but it is not enough by itself for an automotive harness. For real vehicle use, add proper protection, fusing, enclosure, strain relief, and preferably an automotive-grade or isolated CAN interface.

## Initial Wiring

Use this as the first bench wiring plan. Adjust GPIOs if the board silk screen or firmware framework reserves them.

| ESP32-S3 board | SN65HVD230 module | Direction | Notes |
| --- | --- | --- | --- |
| `3V3` | `VCC` / `3V3` | Power | Power the module from 3.3 V, not 5 V. |
| `GND` | `GND` | Power/reference | Share ground with the CAN adapter or the other CAN node. |
| `GPIO4` as TWAI TX | `CTX` / `TXD` / `D` | ESP32 -> transceiver | Proposed default. TWAI TX is configurable in firmware. |
| `GPIO5` as TWAI RX | `CRX` / `RXD` / `R` | Transceiver -> ESP32 | Proposed default. TWAI RX is configurable in firmware. |
| Bus `CANH` | `CANH` | CAN bus | Use twisted pair with `CANL` for longer wiring. |
| Bus `CANL` | `CANL` | CAN bus | Keep polarity consistent across all nodes. |
| Optional bus ground | `GND` terminal, if present | Reference | Recommended for bench testing between USB-CAN adapter and ESP32 node. |
| `GND` or onboard setting | `RS` / `S`, if exposed | Mode select | Low selects high-speed mode on SN65HVD230. Many modules already handle this. |

Avoid using ESP32-S3 pins that are tied to boot mode, native USB, onboard LEDs, flash/PSRAM, or board-specific functions unless the board documentation confirms they are safe for general I/O.

## CAN Bus Setup Notes

- Put a 120 ohm termination resistor at each physical end of the CAN bus.
- Do not add termination to every module if more than two nodes are connected.
- Many SN65HVD230 modules include a 120 ohm resistor or solder jumper. Verify the delivered module visually or with a multimeter.
- With the bus unpowered and exactly two 120 ohm terminators installed, measuring between CANH and CANL should read roughly 60 ohm.
- Keep CANH/CANL wiring short for first tests.
- If frames are missing, check bitrate, common ground, CANH/CANL polarity, TX/RX mapping, termination, and the `RS`/mode setting.
- In normal CAN mode, transmitted frames need an ACK from another active node. Listen-only firmware may be useful for observing traffic, but it may not ACK frames from the USB-CAN sender.

## Firmware Expectations

The ESP32-S3 firmware should:

- Configure ESP-IDF TWAI, or an Arduino/PlatformIO wrapper around TWAI.
- Use the same bitrate as the SocketCAN `can0` interface.
- Start with standard 11-bit CAN identifiers.
- Decode the simulator's current CAN IDs:

| CAN ID | Message | Period in simulator |
| --- | --- | --- |
| `0x100` | VehicleMotion | 100 ms |
| `0x101` | EngineStatus | 100 ms |
| `0x102` | PowerStatus | 500 ms |
| `0x103` | GearStatus | 100 ms |
| `0x700` | Diagnostics | 1000 ms |

The firmware should eventually use `config/can-signals.yaml` as the source of truth for IDs, signal names, scaling, and message periods.

## Linux SocketCAN Bench Test

Once a USB-CAN adapter is selected and appears as `can0` on Linux:

```bash
sudo ip link set can0 down || true
sudo ip link set can0 type can bitrate 500000
sudo ip link set can0 up
```

Use the same bitrate in the ESP32-S3 TWAI configuration.

In one terminal:

```bash
candump can0
```

In the project repository:

```bash
./build/cansim --scenario cruise --duration 10 --output socketcan --interface can0
```

Expected result: `candump` shows the simulator frames, and the ESP32-S3 serial log shows the same CAN IDs and decoded values.

The project bitrate is not fixed yet. `500000` is a conservative bench default; `1000000` is also within the advertised SN65HVD230 module capability if every node and adapter is configured for it.

## Bring-Up Checklist

- Confirm the ESP32-S3 variant on the module label: `N16R8` means 16 MB flash and 8 MB PSRAM for ESP32-S3-WROOM-1; `N8R8` means 8 MB flash and 8 MB PSRAM.
- Confirm which USB-C port is native USB and which is UART/download on the delivered board.
- Confirm the expansion board pin labels before wiring GPIO4/GPIO5.
- Check whether the SN65HVD230 module has onboard termination.
- Check whether the SN65HVD230 module exposes `RS` / `S`, or whether high-speed mode is hardwired.
- Validate on a short bench bus before connecting to any vehicle harness.

## Sources

- Texas Instruments SN65HVD23x 3.3 V CAN bus transceiver datasheet: <https://www.ti.com/lit/ds/symlink/sn65hvd230.pdf>
- Espressif ESP-IDF TWAI documentation for ESP32-S3: <https://docs.espressif.com/projects/esp-idf/en/v5.4.4/esp32s3/api-reference/peripherals/twai.html>
- Espressif ESP32-S3-WROOM-1 / WROOM-1U datasheet: <https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf>
