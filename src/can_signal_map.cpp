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

struct PendingMessage {
    bool active{false};
    int start_line{};
    std::optional<std::uint32_t> can_id{};
    std::optional<std::chrono::milliseconds> cycle{};
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
    return value.find_first_not_of(' ');
}

[[nodiscard]] bool startsWith(std::string_view value, std::string_view prefix) {
    return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
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

[[nodiscard]] std::string valueAfterKey(std::string_view trimmed_line, std::string_view key) {
    return trim(trimmed_line.substr(key.size()));
}

void finishMessage(const std::filesystem::path& signal_map_path, PendingMessage& pending,
                   std::vector<CanMessageSchedule>& schedule, std::set<std::uint32_t>& seen_ids) {
    if (!pending.active) {
        return;
    }

    if (!pending.can_id.has_value() || !pending.cycle.has_value()) {
        throw std::runtime_error("CAN signal map message starting at line " +
                                 std::to_string(pending.start_line) +
                                 " is missing id or cycle_ms: " + signal_map_path.string());
    }

    if (!seen_ids.insert(*pending.can_id).second) {
        throw std::runtime_error("Duplicate CAN ID in signal map: " +
                                 std::to_string(*pending.can_id));
    }

    schedule.push_back(CanMessageSchedule{
        .can_id = *pending.can_id,
        .cycle = *pending.cycle,
    });
    pending = PendingMessage{};
}

} // namespace

std::vector<CanMessageSchedule>
loadCanMessageSchedule(const std::filesystem::path& signal_map_path) {
    std::ifstream input{signal_map_path};
    if (!input) {
        throw std::runtime_error("Failed to open CAN signal map: " + signal_map_path.string());
    }

    std::vector<CanMessageSchedule> schedule;
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
            finishMessage(signal_map_path, pending, schedule, seen_ids);
            pending.active = true;
            pending.start_line = line_number;
            continue;
        }

        if (!pending.active || indent != 4) {
            continue;
        }

        if (startsWith(trimmed_line, "id:")) {
            const std::optional<unsigned long> parsed =
                parseUnsigned(valueAfterKey(trimmed_line, "id:"));
            if (!parsed.has_value() || *parsed > std::numeric_limits<std::uint32_t>::max()) {
                throw std::runtime_error("Invalid CAN ID at line " + std::to_string(line_number) +
                                         ": " + signal_map_path.string());
            }

            pending.can_id = static_cast<std::uint32_t>(*parsed);
            continue;
        }

        if (startsWith(trimmed_line, "cycle_ms:")) {
            const std::optional<unsigned long> parsed =
                parseUnsigned(valueAfterKey(trimmed_line, "cycle_ms:"));
            if (!parsed.has_value() || *parsed == 0 ||
                *parsed > static_cast<unsigned long>(std::numeric_limits<int>::max())) {
                throw std::runtime_error("Invalid cycle_ms at line " + std::to_string(line_number) +
                                         ": " + signal_map_path.string());
            }

            pending.cycle = std::chrono::milliseconds{static_cast<int>(*parsed)};
        }
    }

    finishMessage(signal_map_path, pending, schedule, seen_ids);

    if (schedule.empty()) {
        throw std::runtime_error("CAN signal map contains no scheduled messages: " +
                                 signal_map_path.string());
    }

    return schedule;
}

} // namespace cansim
