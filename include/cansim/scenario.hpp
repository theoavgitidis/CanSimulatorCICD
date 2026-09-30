#pragma once

#include "cansim/vehicle_state.hpp"

#include <optional>
#include <string_view>

namespace cansim {

enum class Scenario {
    idle,
    cruise,
    braking,
};

[[nodiscard]] std::optional<Scenario> parseScenario(std::string_view name);
[[nodiscard]] VehicleState vehicleStateForScenario(Scenario scenario);

}  // namespace cansim
