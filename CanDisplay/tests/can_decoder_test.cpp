#include "candisplay/can_decoder.hpp"
#include "candisplay/telemetry_state.hpp"

#include <cmath>
#include <cstdint>
#include <iostream>

namespace {

bool expectTrue(const char* name, bool actual) {
    if (actual) {
        return true;
    }

    std::cerr << name << " expected true\n";
    return false;
}

bool expectFalse(const char* name, bool actual) {
    if (!actual) {
        return true;
    }

    std::cerr << name << " expected false\n";
    return false;
}

bool expectInt(const char* name, int actual, int expected) {
    if (actual == expected) {
        return true;
    }

    std::cerr << name << " expected " << expected << " but got " << actual << '\n';
    return false;
}

bool expectDouble(const char* name, double actual, double expected) {
    if (std::abs(actual - expected) < 0.001) {
        return true;
    }

    std::cerr << name << " expected " << expected << " but got " << actual << '\n';
    return false;
}

bool testDecodesSimulatorFrames() {
    candisplay::TelemetryState state{};

    bool passed = true;
    passed &= expectTrue("vehicle motion decoded",
                         candisplay::decodeCanFrame(
                             candisplay::CanFrameView{
                                 .id = 0x100,
                                 .dlc = 8,
                                 .data = {0x10, 0x27, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00},
                             },
                             state));
    passed &= expectTrue("engine status decoded",
                         candisplay::decodeCanFrame(
                             candisplay::CanFrameView{
                                 .id = 0x101,
                                 .dlc = 8,
                                 .data = {0xB8, 0x0B, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00},
                             },
                             state));
    passed &= expectTrue("power status decoded",
                         candisplay::decodeCanFrame(
                             candisplay::CanFrameView{
                                 .id = 0x102,
                                 .dlc = 8,
                                 .data = {0x4B, 0xEC, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00},
                             },
                             state));
    passed &= expectTrue("gear status decoded",
                         candisplay::decodeCanFrame(
                             candisplay::CanFrameView{
                                 .id = 0x103,
                                 .dlc = 8,
                                 .data = {0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                             },
                             state));
    passed &= expectTrue("diagnostics decoded",
                         candisplay::decodeCanFrame(
                             candisplay::CanFrameView{
                                 .id = 0x700,
                                 .dlc = 8,
                                 .data = {0x01, 0x20, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00},
                             },
                             state));

    passed &= expectDouble("speed", state.speed_kph, 100.0);
    passed &= expectInt("throttle", state.throttle_percent, 50);
    passed &= expectFalse("brake", state.brake_active);
    passed &= expectInt("rpm", state.engine_rpm, 3000);
    passed &= expectInt("engine temperature", state.engine_temp_c, 90);
    passed &= expectInt("fuel", state.fuel_percent, 75);
    passed &= expectDouble("battery voltage", state.battery_voltage, 12.6);
    passed &= expectInt("gear", state.gear, 3);
    passed &= expectTrue("fault active", state.fault_active);
    passed &= expectInt("active dtc", state.active_dtc, 0x0420);

    return passed;
}

bool testRejectsUnknownOrShortFrames() {
    candisplay::TelemetryState state{};

    bool passed = true;
    passed &= expectFalse("unknown CAN ID", candisplay::decodeCanFrame(
                                                candisplay::CanFrameView{
                                                    .id = 0x321,
                                                    .dlc = 8,
                                                },
                                                state));
    passed &= expectFalse("short vehicle motion", candisplay::decodeCanFrame(
                                                      candisplay::CanFrameView{
                                                          .id = 0x100,
                                                          .dlc = 3,
                                                      },
                                                      state));
    return passed;
}

} // namespace

int main() {
    bool passed = true;
    passed &= testDecodesSimulatorFrames();
    passed &= testRejectsUnknownOrShortFrames();

    return passed ? 0 : 1;
}
