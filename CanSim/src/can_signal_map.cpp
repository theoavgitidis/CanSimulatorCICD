#include "cansim/can_signal_map.hpp"

#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>

namespace cansim {
namespace {

struct PendingSignal {
    bool active{false};
    int start_line{};
    CanSignalDefinition definition{};
    bool has_start_byte{false};
    bool has_length_bytes{false};
};

struct PendingMessage {
    bool active{false};
    int start_line{};
    CanMessageDefinition definition{};
    bool has_can_id{false};
    bool has_cycle{false};
    PendingSignal signal{};
};

[[nodiscard]] std::string trim(std::string_view value) {
    const std::size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) {
        return {};
    }

    const std::size_t last = value.find_last_not_of(" \t\r\n");
    return std::string{value.substr(first, last - first + 1)};
}

[[nodiscard]] std::size_t leadingSpaces(std::string_view value) {
    const std::size_t first_non_space = value.find_first_not_of(' ');
    return first_non_space == std::string_view::npos ? value.size() : first_non_space;
}

[[nodiscard]] bool startsWith(std::string_view value, std::string_view prefix) {
    return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
}

[[nodiscard]] std::string valueAfterKey(std::string_view trimmed_line, std::string_view key) {
    return trim(trimmed_line.substr(key.size()));
}

[[nodiscard]] std::optional<unsigned long> parseUnsigned(std::string_view value) {
    const std::string text = trim(value);
    char* end{};
    errno = 0;
    const unsigned long parsed = std::strtoul(text.c_str(), &end, 0);

    if (end == text.c_str() || *end != '\0' || errno == ERANGE) {
        return std::nullopt;
    }

    return parsed;
}

[[nodiscard]] std::optional<double> parseDouble(std::string_view value) {
    const std::string text = trim(value);
    char* end{};
    errno = 0;
    const double parsed = std::strtod(text.c_str(), &end);

    if (end == text.c_str() || *end != '\0' || errno == ERANGE) {
        return std::nullopt;
    }

    return parsed;
}

[[nodiscard]] CanSignalEndian parseEndian(std::string_view value, int line_number,
                                          const std::filesystem::path& signal_map_path) {
    const std::string text = trim(value);
    if (text == "little") {
        return CanSignalEndian::little;
    }

    if (text == "big") {
        return CanSignalEndian::big;
    }

    throw std::runtime_error("Invalid endian at line " + std::to_string(line_number) + ": " +
                             signal_map_path.string());
}

[[nodiscard]] int parseRequiredInt(std::string_view value, int line_number,
                                   const std::filesystem::path& signal_map_path,
                                   std::string_view field_name) {
    const std::optional<unsigned long> parsed = parseUnsigned(value);
    if (!parsed.has_value() ||
        *parsed > static_cast<unsigned long>(std::numeric_limits<int>::max())) {
        throw std::runtime_error("Invalid " + std::string(field_name) + " at line " +
                                 std::to_string(line_number) + ": " + signal_map_path.string());
    }

    return static_cast<int>(*parsed);
}

[[nodiscard]] double parseRequiredDouble(std::string_view value, int line_number,
                                         const std::filesystem::path& signal_map_path,
                                         std::string_view field_name) {
    const std::optional<double> parsed = parseDouble(value);
    if (!parsed.has_value()) {
        throw std::runtime_error("Invalid " + std::string(field_name) + " at line " +
                                 std::to_string(line_number) + ": " + signal_map_path.string());
    }

    return *parsed;
}

void finishSignal(const std::filesystem::path& signal_map_path, PendingMessage& message) {
    PendingSignal& signal = message.signal;
    if (!signal.active) {
        return;
    }

    if (signal.definition.name.empty() || !signal.has_start_byte || !signal.has_length_bytes) {
        throw std::runtime_error(
            "CAN signal map signal starting at line " + std::to_string(signal.start_line) +
            " is missing name, start_byte, or length_bytes: " + signal_map_path.string());
    }

    if (signal.definition.start_byte < 0 || signal.definition.start_byte >= 8 ||
        signal.definition.length_bytes <= 0 || signal.definition.length_bytes > 8 ||
        signal.definition.start_byte + signal.definition.length_bytes > 8) {
        throw std::runtime_error("CAN signal map signal starting at line " +
                                 std::to_string(signal.start_line) +
                                 " has an invalid byte range: " + signal_map_path.string());
    }

    if (signal.definition.scale <= 0.0) {
        throw std::runtime_error("CAN signal map signal starting at line " +
                                 std::to_string(signal.start_line) +
                                 " has invalid non-positive scale: " + signal_map_path.string());
    }

    message.definition.signals.push_back(signal.definition);
    signal = PendingSignal{};
}

void finishMessage(const std::filesystem::path& signal_map_path, PendingMessage& pending,
                   CanSignalMap& signal_map, std::set<std::uint32_t>& seen_ids) {
    if (!pending.active) {
        return;
    }

    finishSignal(signal_map_path, pending);

    if (pending.definition.name.empty() || !pending.has_can_id || !pending.has_cycle) {
        throw std::runtime_error("CAN signal map message starting at line " +
                                 std::to_string(pending.start_line) +
                                 " is missing name, id, or cycle_ms: " + signal_map_path.string());
    }

    if (pending.definition.signals.empty()) {
        throw std::runtime_error("CAN signal map message starting at line " +
                                 std::to_string(pending.start_line) +
                                 " has no signals: " + signal_map_path.string());
    }

    if (!seen_ids.insert(pending.definition.can_id).second) {
        throw std::runtime_error("Duplicate CAN ID in signal map: " +
                                 std::to_string(pending.definition.can_id));
    }

    signal_map.messages.push_back(pending.definition);
    pending = PendingMessage{};
}

} // namespace

CanSignalMap loadCanSignalMap(const std::filesystem::path& signal_map_path) {
    std::ifstream input{signal_map_path};
    if (!input) {
        throw std::runtime_error("Failed to open CAN signal map: " + signal_map_path.string());
    }

    CanSignalMap signal_map;
    std::set<std::uint32_t> seen_ids;
    PendingMessage pending;
    std::string line;
    int line_number = 0;

    while (std::getline(input, line)) {
        ++line_number;

        const std::size_t comment_index = line.find('#');
        if (comment_index != std::string::npos) {
            line.erase(comment_index);
        }

        const std::size_t indent = leadingSpaces(line);
        const std::string trimmed_line = trim(line);
        if (trimmed_line.empty()) {
            continue;
        }

        if (indent == 2 && startsWith(trimmed_line, "- name:")) {
            finishMessage(signal_map_path, pending, signal_map, seen_ids);
            pending.active = true;
            pending.start_line = line_number;
            pending.definition.name = valueAfterKey(trimmed_line, "- name:");
            continue;
        }

        if (indent == 6 && startsWith(trimmed_line, "- name:")) {
            if (!pending.active) {
                throw std::runtime_error("Signal declared before message at line " +
                                         std::to_string(line_number) + ": " +
                                         signal_map_path.string());
            }

            finishSignal(signal_map_path, pending);
            pending.signal.active = true;
            pending.signal.start_line = line_number;
            pending.signal.definition.name = valueAfterKey(trimmed_line, "- name:");
            continue;
        }

        if (!pending.active) {
            continue;
        }

        if (indent == 4) {
            if (startsWith(trimmed_line, "id:")) {
                const std::optional<unsigned long> parsed =
                    parseUnsigned(valueAfterKey(trimmed_line, "id:"));
                if (!parsed.has_value() || *parsed > std::numeric_limits<std::uint32_t>::max()) {
                    throw std::runtime_error("Invalid CAN ID at line " +
                                             std::to_string(line_number) + ": " +
                                             signal_map_path.string());
                }

                pending.definition.can_id = static_cast<std::uint32_t>(*parsed);
                pending.has_can_id = true;
                continue;
            }

            if (startsWith(trimmed_line, "cycle_ms:")) {
                const int cycle_ms = parseRequiredInt(valueAfterKey(trimmed_line, "cycle_ms:"),
                                                      line_number, signal_map_path, "cycle_ms");
                if (cycle_ms <= 0) {
                    throw std::runtime_error("Invalid cycle_ms at line " +
                                             std::to_string(line_number) + ": " +
                                             signal_map_path.string());
                }

                pending.definition.cycle = std::chrono::milliseconds{cycle_ms};
                pending.has_cycle = true;
                continue;
            }
        }

        if (!pending.signal.active) {
            continue;
        }

        if (indent == 8) {
            if (startsWith(trimmed_line, "start_byte:")) {
                pending.signal.definition.start_byte =
                    parseRequiredInt(valueAfterKey(trimmed_line, "start_byte:"), line_number,
                                     signal_map_path, "start_byte");
                pending.signal.has_start_byte = true;
                continue;
            }

            if (startsWith(trimmed_line, "length_bytes:")) {
                pending.signal.definition.length_bytes =
                    parseRequiredInt(valueAfterKey(trimmed_line, "length_bytes:"), line_number,
                                     signal_map_path, "length_bytes");
                pending.signal.has_length_bytes = true;
                continue;
            }

            if (startsWith(trimmed_line, "endian:")) {
                pending.signal.definition.endian = parseEndian(
                    valueAfterKey(trimmed_line, "endian:"), line_number, signal_map_path);
                continue;
            }

            if (startsWith(trimmed_line, "scale:")) {
                pending.signal.definition.scale = parseRequiredDouble(
                    valueAfterKey(trimmed_line, "scale:"), line_number, signal_map_path, "scale");
                continue;
            }

            if (startsWith(trimmed_line, "offset:")) {
                pending.signal.definition.offset = parseRequiredDouble(
                    valueAfterKey(trimmed_line, "offset:"), line_number, signal_map_path, "offset");
                continue;
            }

            if (startsWith(trimmed_line, "minimum:")) {
                pending.signal.definition.minimum =
                    parseRequiredDouble(valueAfterKey(trimmed_line, "minimum:"), line_number,
                                        signal_map_path, "minimum");
                continue;
            }

            if (startsWith(trimmed_line, "maximum:")) {
                pending.signal.definition.maximum =
                    parseRequiredDouble(valueAfterKey(trimmed_line, "maximum:"), line_number,
                                        signal_map_path, "maximum");
                continue;
            }
        }

        if (indent == 10 && startsWith(trimmed_line, "raw_value:")) {
            pending.signal.definition.raw_value_expression =
                valueAfterKey(trimmed_line, "raw_value:");
        }
    }

    finishMessage(signal_map_path, pending, signal_map, seen_ids);

    if (signal_map.messages.empty()) {
        throw std::runtime_error("CAN signal map contains no scheduled messages: " +
                                 signal_map_path.string());
    }

    return signal_map;
}

std::vector<CanMessageSchedule> messageSchedule(const CanSignalMap& signal_map) {
    std::vector<CanMessageSchedule> schedule;
    schedule.reserve(signal_map.messages.size());

    for (const CanMessageDefinition& message : signal_map.messages) {
        schedule.push_back(CanMessageSchedule{
            .can_id = message.can_id,
            .cycle = message.cycle,
        });
    }

    return schedule;
}

std::vector<CanMessageSchedule>
loadCanMessageSchedule(const std::filesystem::path& signal_map_path) {
    return messageSchedule(loadCanSignalMap(signal_map_path));
}

} // namespace cansim
