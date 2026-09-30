#include "cansim/can_encoder.hpp"
#include "cansim/vehicle_state.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

namespace {

bool expectEqual(const char* name, std::uint32_t actual, std::uint32_t expected) {
    if (actual == expected) {
        return true;
    }

    std::cerr << name << " expected " << expected << " but got " << actual << '\n';
    return false;
}

} // namespace

int main() {
    const cansim::VehicleState state{
        .speed_kph = 100.0,
        .throttle_percent = 50.0,
        .brake_active = false,
        .engine_rpm = 3000,
        .engine_temp_c = 90.0,
        .fuel_percent = 75.0,
        .battery_voltage = 12.6,
        .gear = 3,
        .active_dtc = 0x0420,
    };

    const cansim::CanEncoder encoder;
    const cansim::CanFrame motion_frame = encoder.encodeVehicleMotion(state);
    const cansim::CanFrame engine_frame = encoder.encodeEngineStatus(state);
    const cansim::CanFrame power_frame = encoder.encodePowerStatus(state);
    const cansim::CanFrame gear_frame = encoder.encodeGearStatus(state);
    const cansim::CanFrame diagnostics_frame = encoder.encodeDiagnostics(state);
    const std::vector<cansim::CanFrame> all_frames = encoder.encodeAll(state);

    bool passed = true;
    passed &= expectEqual("motion CAN ID", motion_frame.id, 0x100);
    passed &= expectEqual("motion DLC", motion_frame.dlc, 8);
    passed &= expectEqual("speed byte 0", motion_frame.data[0], 0x10);
    passed &= expectEqual("speed byte 1", motion_frame.data[1], 0x27);
    passed &= expectEqual("throttle byte", motion_frame.data[2], 0x32);
    passed &= expectEqual("brake byte", motion_frame.data[3], 0x00);

    passed &= expectEqual("engine CAN ID", engine_frame.id, 0x101);
    passed &= expectEqual("engine DLC", engine_frame.dlc, 8);
    passed &= expectEqual("rpm byte 0", engine_frame.data[0], 0xB8);
    passed &= expectEqual("rpm byte 1", engine_frame.data[1], 0x0B);
    passed &= expectEqual("engine temperature byte", engine_frame.data[2], 0x5A);

    passed &= expectEqual("power CAN ID", power_frame.id, 0x102);
    passed &= expectEqual("power DLC", power_frame.dlc, 8);
    passed &= expectEqual("fuel byte", power_frame.data[0], 0x4B);
    passed &= expectEqual("battery byte 0", power_frame.data[1], 0xEC);
    passed &= expectEqual("battery byte 1", power_frame.data[2], 0x04);

    passed &= expectEqual("gear CAN ID", gear_frame.id, 0x103);
    passed &= expectEqual("gear DLC", gear_frame.dlc, 8);
    passed &= expectEqual("gear byte", gear_frame.data[0], 0x04);

    passed &= expectEqual("diagnostics CAN ID", diagnostics_frame.id, 0x700);
    passed &= expectEqual("diagnostics DLC", diagnostics_frame.dlc, 8);
    passed &= expectEqual("diagnostics active byte", diagnostics_frame.data[0], 0x01);
    passed &= expectEqual("diagnostics code byte 0", diagnostics_frame.data[1], 0x20);
    passed &= expectEqual("diagnostics code byte 1", diagnostics_frame.data[2], 0x04);

    passed &= expectEqual("encodeAll frame count", all_frames.size(), 5);
    passed &= expectEqual("encodeAll frame 0 ID", all_frames[0].id, 0x100);
    passed &= expectEqual("encodeAll frame 1 ID", all_frames[1].id, 0x101);
    passed &= expectEqual("encodeAll frame 2 ID", all_frames[2].id, 0x102);
    passed &= expectEqual("encodeAll frame 3 ID", all_frames[3].id, 0x103);
    passed &= expectEqual("encodeAll frame 4 ID", all_frames[4].id, 0x700);

    return passed ? 0 : 1;
}
