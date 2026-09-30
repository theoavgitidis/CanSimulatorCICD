#include "cansim/can_encoder.hpp"
#include "cansim/vehicle_state.hpp"

#include <cstddef>
#include <iomanip>
#include <iostream>

int main() {
    const cansim::VehicleState state{
        .speed_kph = 100.0,
        .throttle_percent = 50.0,
        .brake_active = false,
    };

    const cansim::CanEncoder encoder;
    const cansim::CanFrame frame = encoder.encodeVehicleMotion(state);

    std::cout << "0x" << std::hex << std::uppercase << frame.id << " [" << std::dec
              << static_cast<int>(frame.dlc) << "]";

    for (std::size_t i = 0; i < frame.dlc; ++i) {
        std::cout << ' ' << std::hex << std::uppercase << std::setw(2)
                  << std::setfill('0') << static_cast<int>(frame.data[i]);
    }

    std::cout << '\n';
    return 0;
}
