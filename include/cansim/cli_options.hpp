#pragma once

#include "cansim/vehicle_state.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace cansim {

enum class OutputMode {
    console,
    socketcan,
};

struct CliOptions {
    double speed_kph{100.0};
    double throttle_percent{50.0};
    bool brake_active{false};
    int engine_rpm{3000};
    double engine_temp_c{90.0};
    double fuel_percent{75.0};
    double battery_voltage{12.6};
    int gear{3};
    std::uint16_t active_dtc{0};
    int frames{1};
    std::optional<double> duration_seconds{};
    std::optional<int> period_ms{};
    OutputMode output_mode{OutputMode::console};
    std::string interface_name{"vcan0"};
    std::string signal_map_path{"config/can-signals.yaml"};
};

enum class CliParseStatus {
    ok,
    help,
    error,
};

struct CliParseResult {
    CliParseStatus status{CliParseStatus::ok};
    CliOptions options{};
    std::string message{};
};

[[nodiscard]] CliParseResult parseCliOptions(int argc, char* argv[]);
[[nodiscard]] VehicleState toVehicleState(const CliOptions& options);
[[nodiscard]] std::string cliUsage();

} // namespace cansim
