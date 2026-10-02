#pragma once

#include <cstdint>

namespace candisplay {

struct TelemetryState {
    double speed_kph{0.0};
    int throttle_percent{0};
    bool brake_active{false};
    int engine_rpm{0};
    int engine_temp_c{0};
    int fuel_percent{0};
    double battery_voltage{0.0};
    int gear{0};
    bool fault_active{false};
    std::uint16_t active_dtc{0};
};

} // namespace candisplay
