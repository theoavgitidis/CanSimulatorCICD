#pragma once

#include <cstdint>

namespace cansim {

struct VehicleState {
    double speed_kph{0.0};
    double throttle_percent{0.0};
    bool brake_active{false};
    int engine_rpm{0};
    double engine_temp_c{0.0};
    double fuel_percent{0.0};
    double battery_voltage{0.0};
    int gear{0};
    std::uint16_t active_dtc{0};
};

}  // namespace cansim
