#include "cansim/can_encoder.hpp"
#include "cansim/vehicle_state.hpp"

#include <cstdint>
#include <iostream>

namespace {

bool expectEqual(const char* name, std::uint32_t actual, std::uint32_t expected) {
    if (actual == expected) {
        return true;
    }

    std::cerr << name << " expected " << expected << " but got " << actual << '\n';
    return false;
}

}  // namespace

int main() {
    const cansim::VehicleState state{
        .speed_kph = 100.0,
        .throttle_percent = 50.0,
        .brake_active = false,
    };

    const cansim::CanEncoder encoder;
    const cansim::CanFrame frame = encoder.encodeVehicleMotion(state);

    bool passed = true;
    passed &= expectEqual("CAN ID", frame.id, 0x100);
    passed &= expectEqual("DLC", frame.dlc, 8);
    passed &= expectEqual("speed byte 0", frame.data[0], 0x10);
    passed &= expectEqual("speed byte 1", frame.data[1], 0x27);
    passed &= expectEqual("throttle byte", frame.data[2], 0x32);
    passed &= expectEqual("brake byte", frame.data[3], 0x00);

    return passed ? 0 : 1;
}
