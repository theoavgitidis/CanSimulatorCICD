#include "cansim/scenario.hpp"

namespace cansim {

std::optional<Scenario> parseScenario(std::string_view name) {
    if (name == "idle") {
        return Scenario::idle;
    }

    if (name == "cruise") {
        return Scenario::cruise;
    }

    if (name == "braking") {
        return Scenario::braking;
    }

    return std::nullopt;
}

VehicleState vehicleStateForScenario(Scenario scenario) {
    switch (scenario) {
        case Scenario::idle:
            return VehicleState{
                .speed_kph = 0.0,
                .throttle_percent = 0.0,
                .brake_active = false,
                .engine_rpm = 800,
                .engine_temp_c = 70.0,
                .fuel_percent = 75.0,
                .battery_voltage = 12.4,
                .gear = 0,
                .active_dtc = 0,
            };

        case Scenario::cruise:
            return VehicleState{
                .speed_kph = 100.0,
                .throttle_percent = 35.0,
                .brake_active = false,
                .engine_rpm = 2500,
                .engine_temp_c = 90.0,
                .fuel_percent = 75.0,
                .battery_voltage = 12.6,
                .gear = 5,
                .active_dtc = 0,
            };

        case Scenario::braking:
            return VehicleState{
                .speed_kph = 40.0,
                .throttle_percent = 0.0,
                .brake_active = true,
                .engine_rpm = 1500,
                .engine_temp_c = 88.0,
                .fuel_percent = 74.0,
                .battery_voltage = 12.5,
                .gear = 3,
                .active_dtc = 0,
            };
    }

    return VehicleState{};
}

}  // namespace cansim
