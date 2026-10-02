#include "candisplay/can_decoder.hpp"

#include <cstddef>
#include <cstdint>

namespace candisplay {
namespace {

constexpr std::uint32_t kVehicleMotionCanId = 0x100;
constexpr std::uint32_t kEngineStatusCanId = 0x101;
constexpr std::uint32_t kPowerStatusCanId = 0x102;
constexpr std::uint32_t kGearStatusCanId = 0x103;
constexpr std::uint32_t kDiagnosticsCanId = 0x700;
constexpr double kSpeedScale = 0.01;
constexpr double kVoltageScale = 0.01;

[[nodiscard]] bool hasPayloadBytes(const CanFrameView& frame, std::uint8_t required_bytes) {
    return frame.dlc >= required_bytes &&
           static_cast<std::size_t>(required_bytes) <= frame.data.size();
}

[[nodiscard]] std::uint16_t readLittleEndianU16(const CanFrameView& frame,
                                                std::uint8_t start_byte) {
    return static_cast<std::uint16_t>(frame.data[start_byte]) |
           static_cast<std::uint16_t>(frame.data[start_byte + 1] << 8U);
}

} // namespace

bool decodeCanFrame(const CanFrameView& frame, TelemetryState& state) {
    switch (frame.id) {
    case kVehicleMotionCanId:
        if (!hasPayloadBytes(frame, 4)) {
            return false;
        }
        state.speed_kph = static_cast<double>(readLittleEndianU16(frame, 0)) * kSpeedScale;
        state.throttle_percent = frame.data[2];
        state.brake_active = frame.data[3] != 0;
        return true;

    case kEngineStatusCanId:
        if (!hasPayloadBytes(frame, 3)) {
            return false;
        }
        state.engine_rpm = readLittleEndianU16(frame, 0);
        state.engine_temp_c = frame.data[2];
        return true;

    case kPowerStatusCanId:
        if (!hasPayloadBytes(frame, 3)) {
            return false;
        }
        state.fuel_percent = frame.data[0];
        state.battery_voltage = static_cast<double>(readLittleEndianU16(frame, 1)) * kVoltageScale;
        return true;

    case kGearStatusCanId:
        if (!hasPayloadBytes(frame, 1)) {
            return false;
        }
        state.gear = static_cast<int>(frame.data[0]) - 1;
        return true;

    case kDiagnosticsCanId:
        if (!hasPayloadBytes(frame, 3)) {
            return false;
        }
        state.fault_active = frame.data[0] != 0;
        state.active_dtc = readLittleEndianU16(frame, 1);
        return true;

    default:
        return false;
    }
}

} // namespace candisplay
