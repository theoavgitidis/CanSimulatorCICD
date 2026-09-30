#include "cansim/cli_options.hpp"

#include "cansim/scenario.hpp"

#include <cerrno>
#include <cstdlib>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace cansim {
namespace {

struct CliOverrides {
    std::optional<double> speed_kph{};
    std::optional<double> throttle_percent{};
    std::optional<bool> brake_active{};
    std::optional<int> engine_rpm{};
    std::optional<double> engine_temp_c{};
    std::optional<double> fuel_percent{};
    std::optional<double> battery_voltage{};
    std::optional<int> gear{};
    std::optional<std::uint16_t> active_dtc{};
    std::optional<int> frames{};
    std::optional<OutputMode> output_mode{};
    std::optional<std::string> interface_name{};
};

[[nodiscard]] CliParseResult parseError(std::string message) {
    CliParseResult result;
    result.status = CliParseStatus::error;
    result.message = std::move(message);
    return result;
}

[[nodiscard]] CliParseResult parseHelp() {
    CliParseResult result;
    result.status = CliParseStatus::help;
    return result;
}

[[nodiscard]] std::optional<double> parseDouble(const char* value) {
    char* end{};
    errno = 0;
    const double parsed = std::strtod(value, &end);

    if (end == value || *end != '\0' || errno == ERANGE) {
        return std::nullopt;
    }

    return parsed;
}

[[nodiscard]] std::optional<int> parsePositiveInt(const char* value) {
    char* end{};
    errno = 0;
    const long parsed = std::strtol(value, &end, 10);

    if (end == value || *end != '\0' || errno == ERANGE || parsed <= 0 ||
        parsed > std::numeric_limits<int>::max()) {
        return std::nullopt;
    }

    return static_cast<int>(parsed);
}

[[nodiscard]] std::optional<int> parseInt(const char* value) {
    char* end{};
    errno = 0;
    const long parsed = std::strtol(value, &end, 10);

    if (end == value || *end != '\0' || errno == ERANGE ||
        parsed < std::numeric_limits<int>::min() || parsed > std::numeric_limits<int>::max()) {
        return std::nullopt;
    }

    return static_cast<int>(parsed);
}

[[nodiscard]] std::optional<std::uint16_t> parseDtcCode(const char* value) {
    char* end{};
    errno = 0;
    const unsigned long parsed = std::strtoul(value, &end, 0);

    if (end == value || *end != '\0' || errno == ERANGE || parsed > 0xFFFFUL) {
        return std::nullopt;
    }

    return static_cast<std::uint16_t>(parsed);
}

[[nodiscard]] std::optional<bool> parseBool(std::string_view value) {
    if (value == "true" || value == "1" || value == "yes") {
        return true;
    }

    if (value == "false" || value == "0" || value == "no") {
        return false;
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<OutputMode> parseOutputMode(std::string_view value) {
    if (value == "console") {
        return OutputMode::console;
    }

    if (value == "socketcan") {
        return OutputMode::socketcan;
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<const char*>
readNextValue(int argc, char* argv[], int& index, std::string_view option, std::string& error) {
    if (index + 1 >= argc) {
        error = "Missing value for " + std::string(option);
        return std::nullopt;
    }

    ++index;
    return argv[index];
}

void applyScenario(CliOptions& options, Scenario scenario) {
    const VehicleState state = vehicleStateForScenario(scenario);
    options.speed_kph = state.speed_kph;
    options.throttle_percent = state.throttle_percent;
    options.brake_active = state.brake_active;
    options.engine_rpm = state.engine_rpm;
    options.engine_temp_c = state.engine_temp_c;
    options.fuel_percent = state.fuel_percent;
    options.battery_voltage = state.battery_voltage;
    options.gear = state.gear;
    options.active_dtc = state.active_dtc;
}

void applyOverrides(CliOptions& options, const CliOverrides& overrides) {
    if (overrides.speed_kph.has_value()) {
        options.speed_kph = *overrides.speed_kph;
    }

    if (overrides.throttle_percent.has_value()) {
        options.throttle_percent = *overrides.throttle_percent;
    }

    if (overrides.brake_active.has_value()) {
        options.brake_active = *overrides.brake_active;
    }

    if (overrides.engine_rpm.has_value()) {
        options.engine_rpm = *overrides.engine_rpm;
    }

    if (overrides.engine_temp_c.has_value()) {
        options.engine_temp_c = *overrides.engine_temp_c;
    }

    if (overrides.fuel_percent.has_value()) {
        options.fuel_percent = *overrides.fuel_percent;
    }

    if (overrides.battery_voltage.has_value()) {
        options.battery_voltage = *overrides.battery_voltage;
    }

    if (overrides.gear.has_value()) {
        options.gear = *overrides.gear;
    }

    if (overrides.active_dtc.has_value()) {
        options.active_dtc = *overrides.active_dtc;
    }

    if (overrides.frames.has_value()) {
        options.frames = *overrides.frames;
    }

    if (overrides.output_mode.has_value()) {
        options.output_mode = *overrides.output_mode;
    }

    if (overrides.interface_name.has_value()) {
        options.interface_name = *overrides.interface_name;
    }
}

} // namespace

std::string cliUsage() {
    return "Usage: cansim [--scenario idle|cruise|braking] "
           "[--speed KPH] [--throttle PERCENT] "
           "[--brake true|false] [--rpm RPM] [--engine-temp CELSIUS] "
           "[--fuel PERCENT] [--battery VOLTS] [--gear GEAR] "
           "[--dtc CODE] [--frames COUNT] [--output console|socketcan] "
           "[--interface IFACE]\n";
}

CliParseResult parseCliOptions(int argc, char* argv[]) {
    CliOptions options;
    CliOverrides overrides;
    std::optional<Scenario> scenario;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg{argv[i]};

        if (arg == "--help" || arg == "-h") {
            return parseHelp();
        }

        if (arg == "--scenario") {
            std::string error;
            const std::optional<const char*> value = readNextValue(argc, argv, i, arg, error);
            if (!value.has_value()) {
                return parseError(error);
            }

            const std::optional<Scenario> parsed = parseScenario(*value);
            if (!parsed.has_value()) {
                return parseError("Invalid scenario: " + std::string(*value));
            }

            scenario = *parsed;
            continue;
        }

        if (arg == "--speed") {
            std::string error;
            const std::optional<const char*> value = readNextValue(argc, argv, i, arg, error);
            if (!value.has_value()) {
                return parseError(error);
            }

            const std::optional<double> parsed = parseDouble(*value);
            if (!parsed.has_value()) {
                return parseError("Invalid speed: " + std::string(*value));
            }

            overrides.speed_kph = *parsed;
            continue;
        }

        if (arg == "--throttle") {
            std::string error;
            const std::optional<const char*> value = readNextValue(argc, argv, i, arg, error);
            if (!value.has_value()) {
                return parseError(error);
            }

            const std::optional<double> parsed = parseDouble(*value);
            if (!parsed.has_value()) {
                return parseError("Invalid throttle: " + std::string(*value));
            }

            overrides.throttle_percent = *parsed;
            continue;
        }

        if (arg == "--brake") {
            std::string error;
            const std::optional<const char*> value = readNextValue(argc, argv, i, arg, error);
            if (!value.has_value()) {
                return parseError(error);
            }

            const std::optional<bool> parsed = parseBool(*value);
            if (!parsed.has_value()) {
                return parseError("Invalid brake value: " + std::string(*value));
            }

            overrides.brake_active = *parsed;
            continue;
        }

        if (arg == "--rpm") {
            std::string error;
            const std::optional<const char*> value = readNextValue(argc, argv, i, arg, error);
            if (!value.has_value()) {
                return parseError(error);
            }

            const std::optional<int> parsed = parsePositiveInt(*value);
            if (!parsed.has_value()) {
                return parseError("Invalid RPM: " + std::string(*value));
            }

            overrides.engine_rpm = *parsed;
            continue;
        }

        if (arg == "--engine-temp") {
            std::string error;
            const std::optional<const char*> value = readNextValue(argc, argv, i, arg, error);
            if (!value.has_value()) {
                return parseError(error);
            }

            const std::optional<double> parsed = parseDouble(*value);
            if (!parsed.has_value()) {
                return parseError("Invalid engine temperature: " + std::string(*value));
            }

            overrides.engine_temp_c = *parsed;
            continue;
        }

        if (arg == "--fuel") {
            std::string error;
            const std::optional<const char*> value = readNextValue(argc, argv, i, arg, error);
            if (!value.has_value()) {
                return parseError(error);
            }

            const std::optional<double> parsed = parseDouble(*value);
            if (!parsed.has_value()) {
                return parseError("Invalid fuel level: " + std::string(*value));
            }

            overrides.fuel_percent = *parsed;
            continue;
        }

        if (arg == "--battery") {
            std::string error;
            const std::optional<const char*> value = readNextValue(argc, argv, i, arg, error);
            if (!value.has_value()) {
                return parseError(error);
            }

            const std::optional<double> parsed = parseDouble(*value);
            if (!parsed.has_value()) {
                return parseError("Invalid battery voltage: " + std::string(*value));
            }

            overrides.battery_voltage = *parsed;
            continue;
        }

        if (arg == "--gear") {
            std::string error;
            const std::optional<const char*> value = readNextValue(argc, argv, i, arg, error);
            if (!value.has_value()) {
                return parseError(error);
            }

            const std::optional<int> parsed = parseInt(*value);
            if (!parsed.has_value()) {
                return parseError("Invalid gear: " + std::string(*value));
            }

            overrides.gear = *parsed;
            continue;
        }

        if (arg == "--dtc") {
            std::string error;
            const std::optional<const char*> value = readNextValue(argc, argv, i, arg, error);
            if (!value.has_value()) {
                return parseError(error);
            }

            const std::optional<std::uint16_t> parsed = parseDtcCode(*value);
            if (!parsed.has_value()) {
                return parseError("Invalid diagnostic trouble code: " + std::string(*value));
            }

            overrides.active_dtc = *parsed;
            continue;
        }

        if (arg == "--frames") {
            std::string error;
            const std::optional<const char*> value = readNextValue(argc, argv, i, arg, error);
            if (!value.has_value()) {
                return parseError(error);
            }

            const std::optional<int> parsed = parsePositiveInt(*value);
            if (!parsed.has_value()) {
                return parseError("Invalid frame count: " + std::string(*value));
            }

            overrides.frames = *parsed;
            continue;
        }

        if (arg == "--output") {
            std::string error;
            const std::optional<const char*> value = readNextValue(argc, argv, i, arg, error);
            if (!value.has_value()) {
                return parseError(error);
            }

            const std::optional<OutputMode> parsed = parseOutputMode(*value);
            if (!parsed.has_value()) {
                return parseError("Invalid output mode: " + std::string(*value));
            }

            overrides.output_mode = *parsed;
            continue;
        }

        if (arg == "--interface") {
            std::string error;
            const std::optional<const char*> value = readNextValue(argc, argv, i, arg, error);
            if (!value.has_value()) {
                return parseError(error);
            }

            if (std::string_view{*value}.empty()) {
                return parseError("Invalid interface: empty name");
            }

            overrides.interface_name = std::string{*value};
            continue;
        }

        return parseError("Unknown option: " + std::string(arg));
    }

    if (scenario.has_value()) {
        applyScenario(options, *scenario);
    }
    applyOverrides(options, overrides);

    CliParseResult result;
    result.status = CliParseStatus::ok;
    result.options = options;
    return result;
}

VehicleState toVehicleState(const CliOptions& options) {
    return VehicleState{
        .speed_kph = options.speed_kph,
        .throttle_percent = options.throttle_percent,
        .brake_active = options.brake_active,
        .engine_rpm = options.engine_rpm,
        .engine_temp_c = options.engine_temp_c,
        .fuel_percent = options.fuel_percent,
        .battery_voltage = options.battery_voltage,
        .gear = options.gear,
        .active_dtc = options.active_dtc,
    };
}

} // namespace cansim
