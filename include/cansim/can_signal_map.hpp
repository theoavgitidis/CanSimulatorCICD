#pragma once

#include "cansim/can_scheduler.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace cansim {

enum class CanSignalEndian {
    little,
    big,
};

struct CanSignalDefinition {
    std::string name{};
    int start_byte{};
    int length_bytes{};
    CanSignalEndian endian{CanSignalEndian::little};
    double scale{1.0};
    double offset{0.0};
    std::optional<double> minimum{};
    std::optional<double> maximum{};
    std::string raw_value_expression{};
};

struct CanMessageDefinition {
    std::string name{};
    std::uint32_t can_id{};
    std::chrono::milliseconds cycle{};
    std::vector<CanSignalDefinition> signals{};
};

struct CanSignalMap {
    std::vector<CanMessageDefinition> messages{};
};

[[nodiscard]] CanSignalMap loadCanSignalMap(const std::filesystem::path& signal_map_path);
[[nodiscard]] std::vector<CanMessageSchedule> messageSchedule(const CanSignalMap& signal_map);
[[nodiscard]] std::vector<CanMessageSchedule>
loadCanMessageSchedule(const std::filesystem::path& signal_map_path);

} // namespace cansim
