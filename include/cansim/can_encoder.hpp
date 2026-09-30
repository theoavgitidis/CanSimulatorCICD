#pragma once

#include "cansim/can_frame.hpp"
#include "cansim/vehicle_state.hpp"

namespace cansim {

class CanEncoder {
public:
    [[nodiscard]] CanFrame encodeVehicleMotion(const VehicleState& state) const;
    [[nodiscard]] CanFrame encodeEngineStatus(const VehicleState& state) const;
};

}  // namespace cansim
