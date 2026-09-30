#include "cansim/cli_options.hpp"

#include <cerrno>
#include <cstdlib>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

namespace cansim {
namespace {

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

[[nodiscard]] std::optional<const char*> readNextValue(int argc,
                                                       char* argv[],
                                                       int& index,
                                                       std::string_view option,
                                                       std::string& error) {
    if (index + 1 >= argc) {
        error = "Missing value for " + std::string(option);
        return std::nullopt;
    }

    ++index;
    return argv[index];
}

}  // namespace

std::string cliUsage() {
    return "Usage: cansim [--speed KPH] [--throttle PERCENT] "
           "[--brake true|false] [--rpm RPM] [--engine-temp CELSIUS] "
           "[--fuel PERCENT] [--battery VOLTS] [--gear GEAR] "
           "[--dtc CODE] [--frames COUNT]\n";
}

CliParseResult parseCliOptions(int argc, char* argv[]) {
    CliOptions options;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg{argv[i]};

        if (arg == "--help" || arg == "-h") {
            return parseHelp();
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

            options.speed_kph = *parsed;
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

            options.throttle_percent = *parsed;
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

            options.brake_active = *parsed;
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

            options.engine_rpm = *parsed;
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

            options.engine_temp_c = *parsed;
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

            options.fuel_percent = *parsed;
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

            options.battery_voltage = *parsed;
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

            options.gear = *parsed;
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

            options.active_dtc = *parsed;
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

            options.frames = *parsed;
            continue;
        }

        return parseError("Unknown option: " + std::string(arg));
    }

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

}  // namespace cansim
