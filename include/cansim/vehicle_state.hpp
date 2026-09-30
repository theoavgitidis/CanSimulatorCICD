#pragma once

namespace cansim {

struct VehicleState {
    double speed_kph{0.0};
    double throttle_percent{0.0};
    bool brake_active{false};
    int engine_rpm{0};
    double engine_temp_c{0.0};
    double fuel_percent{0.0};
    double battery_voltage{0.0};
};

}  // namespace cansim
