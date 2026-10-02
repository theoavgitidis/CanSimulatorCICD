#pragma once

#include "candisplay/telemetry_state.hpp"

#include <array>
#include <cstdint>

namespace candisplay {

struct CanFrameView {
    std::uint32_t id{0};
    std::uint8_t dlc{0};
    std::array<std::uint8_t, 8> data{};
};

[[nodiscard]] bool decodeCanFrame(const CanFrameView& frame, TelemetryState& state);

} // namespace candisplay
