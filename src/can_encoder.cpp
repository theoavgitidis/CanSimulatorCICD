#include "cansim/can_encoder.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace cansim {
namespace {

constexpr std::uint32_t kVehicleMotionCanId = 0x100;
constexpr std::uint32_t kEngineStatusCanId = 0x101;
constexpr std::uint32_t kPowerStatusCanId = 0x102;
constexpr std::uint32_t kGearStatusCanId = 0x103;
constexpr double kSpeedScale = 0.01;
constexpr double kVoltageScale = 0.01;

[[nodiscard]] std::uint16_t encodeSpeed(double speed_kph) {
    const double clamped = std::clamp(speed_kph, 0.0, 655.35);
    return static_cast<std::uint16_t>(std::lround(clamped / kSpeedScale));
}

[[nodiscard]] std::uint8_t encodeThrottle(double throttle_percent) {
    const double clamped = std::clamp(throttle_percent, 0.0, 100.0);
    return static_cast<std::uint8_t>(std::lround(clamped));
}

[[nodiscard]] std::uint16_t encodeRpm(int engine_rpm) {
    const int clamped = std::clamp(engine_rpm, 0, 16000);
    return static_cast<std::uint16_t>(clamped);
}

[[nodiscard]] std::uint8_t encodeTemperature(double engine_temp_c) {
    const double clamped = std::clamp(engine_temp_c, 0.0, 255.0);
    return static_cast<std::uint8_t>(std::lround(clamped));
}

[[nodiscard]] std::uint8_t encodeFuel(double fuel_percent) {
    const double clamped = std::clamp(fuel_percent, 0.0, 100.0);
    return static_cast<std::uint8_t>(std::lround(clamped));
}

[[nodiscard]] std::uint16_t encodeBatteryVoltage(double battery_voltage) {
    const double clamped = std::clamp(battery_voltage, 0.0, 655.35);
    return static_cast<std::uint16_t>(std::lround(clamped / kVoltageScale));
}

[[nodiscard]] std::uint8_t encodeGear(int gear) {
    const int clamped = std::clamp(gear, -1, 8);
    return static_cast<std::uint8_t>(clamped + 1);
}

}  // namespace

CanFrame CanEncoder::encodeVehicleMotion(const VehicleState& state) const {
    CanFrame frame{};
    frame.id = kVehicleMotionCanId;
    frame.dlc = 8;

    const std::uint16_t speed_raw = encodeSpeed(state.speed_kph);
    frame.data[0] = static_cast<std::uint8_t>(speed_raw & 0xFF);
    frame.data[1] = static_cast<std::uint8_t>((speed_raw >> 8) & 0xFF);
    frame.data[2] = encodeThrottle(state.throttle_percent);
    frame.data[3] = state.brake_active ? 1U : 0U;

    return frame;
}

CanFrame CanEncoder::encodeEngineStatus(const VehicleState& state) const {
    CanFrame frame{};
    frame.id = kEngineStatusCanId;
    frame.dlc = 8;

    const std::uint16_t rpm_raw = encodeRpm(state.engine_rpm);
    frame.data[0] = static_cast<std::uint8_t>(rpm_raw & 0xFF);
    frame.data[1] = static_cast<std::uint8_t>((rpm_raw >> 8) & 0xFF);
    frame.data[2] = encodeTemperature(state.engine_temp_c);

    return frame;
}

CanFrame CanEncoder::encodePowerStatus(const VehicleState& state) const {
    CanFrame frame{};
    frame.id = kPowerStatusCanId;
    frame.dlc = 8;

    const std::uint16_t voltage_raw = encodeBatteryVoltage(state.battery_voltage);
    frame.data[0] = encodeFuel(state.fuel_percent);
    frame.data[1] = static_cast<std::uint8_t>(voltage_raw & 0xFF);
    frame.data[2] = static_cast<std::uint8_t>((voltage_raw >> 8) & 0xFF);

    return frame;
}

CanFrame CanEncoder::encodeGearStatus(const VehicleState& state) const {
    CanFrame frame{};
    frame.id = kGearStatusCanId;
    frame.dlc = 8;

    frame.data[0] = encodeGear(state.gear);

    return frame;
}

}  // namespace cansim
