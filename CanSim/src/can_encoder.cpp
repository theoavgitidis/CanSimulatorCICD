#include "cansim/can_encoder.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace cansim {
namespace {

constexpr std::uint32_t kVehicleMotionCanId = 0x100;
constexpr std::uint32_t kEngineStatusCanId = 0x101;
constexpr std::uint32_t kPowerStatusCanId = 0x102;
constexpr std::uint32_t kGearStatusCanId = 0x103;
constexpr std::uint32_t kDiagnosticsCanId = 0x700;
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

[[nodiscard]] double clampPhysicalValue(double value, const CanSignalDefinition& signal) {
    if (signal.minimum.has_value()) {
        value = std::max(value, *signal.minimum);
    }

    if (signal.maximum.has_value()) {
        value = std::min(value, *signal.maximum);
    }

    return value;
}

[[nodiscard]] std::uint64_t rawLimitForLength(int length_bytes) {
    return length_bytes >= 8 ? std::numeric_limits<std::uint64_t>::max()
                             : ((std::uint64_t{1} << (length_bytes * 8)) - 1U);
}

[[nodiscard]] std::uint64_t scaledRawValue(double value, const CanSignalDefinition& signal) {
    const double clamped = clampPhysicalValue(value, signal);
    const double raw = std::lround((clamped - signal.offset) / signal.scale);

    if (raw < 0.0 || raw > static_cast<double>(rawLimitForLength(signal.length_bytes))) {
        throw std::runtime_error("Encoded signal is out of range: " + signal.name);
    }

    return static_cast<std::uint64_t>(raw);
}

[[nodiscard]] std::uint64_t rawValueForSignal(const CanSignalDefinition& signal,
                                              const VehicleState& state) {
    if (signal.name == "speed_kph") {
        return scaledRawValue(state.speed_kph, signal);
    }

    if (signal.name == "throttle_percent") {
        return scaledRawValue(state.throttle_percent, signal);
    }

    if (signal.name == "brake_active") {
        return state.brake_active ? 1U : 0U;
    }

    if (signal.name == "engine_rpm") {
        return scaledRawValue(static_cast<double>(state.engine_rpm), signal);
    }

    if (signal.name == "engine_temp_c") {
        return scaledRawValue(state.engine_temp_c, signal);
    }

    if (signal.name == "fuel_percent") {
        return scaledRawValue(state.fuel_percent, signal);
    }

    if (signal.name == "battery_voltage") {
        return scaledRawValue(state.battery_voltage, signal);
    }

    if (signal.name == "selected_gear") {
        if (signal.raw_value_expression != "gear + 1") {
            throw std::runtime_error("Unsupported selected_gear encoding: " +
                                     signal.raw_value_expression);
        }

        return encodeGear(state.gear);
    }

    if (signal.name == "fault_active") {
        return state.active_dtc == 0 ? 0U : 1U;
    }

    if (signal.name == "active_dtc") {
        return scaledRawValue(static_cast<double>(state.active_dtc), signal);
    }

    throw std::runtime_error("Unsupported CAN signal: " + signal.name);
}

void writeRawValue(CanFrame& frame, const CanSignalDefinition& signal, std::uint64_t raw_value) {
    if (raw_value > rawLimitForLength(signal.length_bytes)) {
        throw std::runtime_error("Raw signal value does not fit in configured byte length: " +
                                 signal.name);
    }

    for (int i = 0; i < signal.length_bytes; ++i) {
        const int target_byte = signal.endian == CanSignalEndian::little
                                    ? signal.start_byte + i
                                    : signal.start_byte + signal.length_bytes - 1 - i;
        frame.data[static_cast<std::size_t>(target_byte)] =
            static_cast<std::uint8_t>((raw_value >> (i * 8)) & 0xFFU);
    }
}

} // namespace

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

CanFrame CanEncoder::encodeDiagnostics(const VehicleState& state) const {
    CanFrame frame{};
    frame.id = kDiagnosticsCanId;
    frame.dlc = 8;

    frame.data[0] = state.active_dtc == 0 ? 0U : 1U;
    frame.data[1] = static_cast<std::uint8_t>(state.active_dtc & 0xFF);
    frame.data[2] = static_cast<std::uint8_t>((state.active_dtc >> 8) & 0xFF);

    return frame;
}

std::vector<CanFrame> CanEncoder::encodeAll(const VehicleState& state) const {
    return {
        encodeVehicleMotion(state), encodeEngineStatus(state), encodePowerStatus(state),
        encodeGearStatus(state),    encodeDiagnostics(state),
    };
}

SignalMapCanEncoder::SignalMapCanEncoder(CanSignalMap signal_map)
    : signal_map_{std::move(signal_map)} {}

std::vector<CanFrame> SignalMapCanEncoder::encodeAll(const VehicleState& state) const {
    std::vector<CanFrame> frames;
    frames.reserve(signal_map_.messages.size());

    for (const CanMessageDefinition& message : signal_map_.messages) {
        CanFrame frame{};
        frame.id = message.can_id;
        frame.dlc = 8;

        for (const CanSignalDefinition& signal : message.signals) {
            writeRawValue(frame, signal, rawValueForSignal(signal, state));
        }

        frames.push_back(frame);
    }

    return frames;
}

} // namespace cansim
