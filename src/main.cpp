#include "cansim/can_encoder.hpp"
#include "cansim/vehicle_state.hpp"

#include <cerrno>
#include <cstdlib>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string_view>

namespace {

struct CliOptions {
    double speed_kph{100.0};
    double throttle_percent{50.0};
    bool brake_active{false};
    int frames{1};
};

void printUsage() {
    std::cout << "Usage: cansim [--speed KPH] [--throttle PERCENT] "
                 "[--brake true|false] [--frames COUNT]\n";
}

std::optional<double> parseDouble(const char* value) {
    char* end{};
    errno = 0;
    const double parsed = std::strtod(value, &end);

    if (end == value || *end != '\0' || errno == ERANGE) {
        return std::nullopt;
    }

    return parsed;
}

std::optional<int> parsePositiveInt(const char* value) {
    char* end{};
    errno = 0;
    const long parsed = std::strtol(value, &end, 10);

    if (end == value || *end != '\0' || errno == ERANGE || parsed <= 0) {
        return std::nullopt;
    }

    return static_cast<int>(parsed);
}

std::optional<bool> parseBool(std::string_view value) {
    if (value == "true" || value == "1" || value == "yes") {
        return true;
    }

    if (value == "false" || value == "0" || value == "no") {
        return false;
    }

    return std::nullopt;
}

bool readNextValue(int argc, char* argv[], int& index, std::string_view option, const char*& value) {
    if (index + 1 >= argc) {
        std::cerr << "Missing value for " << option << '\n';
        return false;
    }

    ++index;
    value = argv[index];
    return true;
}

std::optional<CliOptions> parseArgs(int argc, char* argv[]) {
    CliOptions options;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg{argv[i]};

        if (arg == "--help" || arg == "-h") {
            printUsage();
            return std::nullopt;
        }

        if (arg == "--speed") {
            const char* value{};
            if (!readNextValue(argc, argv, i, arg, value)) {
                return std::nullopt;
            }

            const std::optional<double> parsed = parseDouble(value);
            if (!parsed.has_value()) {
                std::cerr << "Invalid speed: " << value << '\n';
                return std::nullopt;
            }

            options.speed_kph = *parsed;
            continue;
        }

        if (arg == "--throttle") {
            const char* value{};
            if (!readNextValue(argc, argv, i, arg, value)) {
                return std::nullopt;
            }

            const std::optional<double> parsed = parseDouble(value);
            if (!parsed.has_value()) {
                std::cerr << "Invalid throttle: " << value << '\n';
                return std::nullopt;
            }

            options.throttle_percent = *parsed;
            continue;
        }

        if (arg == "--brake") {
            const char* value{};
            if (!readNextValue(argc, argv, i, arg, value)) {
                return std::nullopt;
            }

            const std::optional<bool> parsed = parseBool(value);
            if (!parsed.has_value()) {
                std::cerr << "Invalid brake value: " << value << '\n';
                return std::nullopt;
            }

            options.brake_active = *parsed;
            continue;
        }

        if (arg == "--frames") {
            const char* value{};
            if (!readNextValue(argc, argv, i, arg, value)) {
                return std::nullopt;
            }

            const std::optional<int> parsed = parsePositiveInt(value);
            if (!parsed.has_value()) {
                std::cerr << "Invalid frame count: " << value << '\n';
                return std::nullopt;
            }

            options.frames = *parsed;
            continue;
        }

        std::cerr << "Unknown option: " << arg << '\n';
        printUsage();
        return std::nullopt;
    }

    return options;
}

void printFrame(const cansim::CanFrame& frame) {
    std::cout << "0x" << std::hex << std::uppercase << frame.id << " [" << std::dec
              << static_cast<int>(frame.dlc) << "]";

    for (std::size_t i = 0; i < frame.dlc; ++i) {
        std::cout << ' ' << std::hex << std::uppercase << std::setw(2)
                  << std::setfill('0') << static_cast<int>(frame.data[i]);
    }

    std::cout << '\n';
}

}  // namespace

int main(int argc, char* argv[]) {
    const std::optional<CliOptions> options = parseArgs(argc, argv);
    if (!options.has_value()) {
        return argc > 1 && (std::string_view{argv[1]} == "--help" || std::string_view{argv[1]} == "-h")
                   ? 0
                   : 1;
    }

    const cansim::VehicleState state{
        .speed_kph = options->speed_kph,
        .throttle_percent = options->throttle_percent,
        .brake_active = options->brake_active,
    };

    const cansim::CanEncoder encoder;
    for (int i = 0; i < options->frames; ++i) {
        printFrame(encoder.encodeVehicleMotion(state));
    }

    return 0;
}
