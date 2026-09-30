#pragma once

#include "cansim/can_frame.hpp"
#include "cansim/can_signal_map.hpp"
#include "cansim/vehicle_state.hpp"

#include <vector>

namespace cansim {

class CanEncoder {
  public:
    [[nodiscard]] CanFrame encodeVehicleMotion(const VehicleState& state) const;
    [[nodiscard]] CanFrame encodeEngineStatus(const VehicleState& state) const;
    [[nodiscard]] CanFrame encodePowerStatus(const VehicleState& state) const;
    [[nodiscard]] CanFrame encodeGearStatus(const VehicleState& state) const;
    [[nodiscard]] CanFrame encodeDiagnostics(const VehicleState& state) const;
    [[nodiscard]] std::vector<CanFrame> encodeAll(const VehicleState& state) const;
};

class SignalMapCanEncoder {
  public:
    explicit SignalMapCanEncoder(CanSignalMap signal_map);

    [[nodiscard]] std::vector<CanFrame> encodeAll(const VehicleState& state) const;

  private:
    CanSignalMap signal_map_;
};

} // namespace cansim
