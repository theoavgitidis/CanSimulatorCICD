#pragma once

namespace cansim {

struct VehicleState {
    double speed_kph{0.0};
    double throttle_percent{0.0};
    bool brake_active{false};
};

}  // namespace cansim
