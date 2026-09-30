#include "cansim/can_signal_map.hpp"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace std::chrono_literals;

bool expectEqual(const char* name, std::size_t actual, std::size_t expected) {
    if (actual == expected) {
        return true;
    }

    std::cerr << name << " expected " << expected << " but got " << actual << '\n';
    return false;
}

bool expectEqual(const char* name, std::uint32_t actual, std::uint32_t expected) {
    if (actual == expected) {
        return true;
    }

    std::cerr << name << " expected " << expected << " but got " << actual << '\n';
    return false;
}

bool expectInt(const char* name, int actual, int expected) {
    if (actual == expected) {
        return true;
    }

    std::cerr << name << " expected " << expected << " but got " << actual << '\n';
    return false;
}

bool expectEqual(const char* name, std::chrono::milliseconds actual,
                 std::chrono::milliseconds expected) {
    if (actual == expected) {
        return true;
    }

    std::cerr << name << " expected " << expected.count() << "ms but got " << actual.count()
              << "ms\n";
    return false;
}

bool expectDouble(const char* name, double actual, double expected) {
    if (std::abs(actual - expected) < 0.0001) {
        return true;
    }

    std::cerr << name << " expected " << expected << " but got " << actual << '\n';
    return false;
}

bool expectString(const char* name, std::string_view actual, std::string_view expected) {
    if (actual == expected) {
        return true;
    }

    std::cerr << name << " expected " << expected << " but got " << actual << '\n';
    return false;
}

bool expectEndian(const char* name, cansim::CanSignalEndian actual,
                  cansim::CanSignalEndian expected) {
    if (actual == expected) {
        return true;
    }

    std::cerr << name << " had unexpected endian\n";
    return false;
}

[[nodiscard]] std::filesystem::path testFilePath(const std::string& filename) {
    return std::filesystem::temp_directory_path() / filename;
}

bool writeFile(const std::filesystem::path& path, const std::string& contents) {
    std::ofstream output{path};
    if (!output) {
        std::cerr << "failed to create " << path << '\n';
        return false;
    }

    output << contents;
    return true;
}

bool testLoadsMessageCycles() {
    const std::filesystem::path path = testFilePath("cansim_signal_map_valid.yaml");
    if (!writeFile(path, "messages:\n"
                         "  - name: VehicleMotion\n"
                         "    id: 0x100\n"
                         "    cycle_ms: 100\n"
                         "    signals:\n"
                         "      - name: speed_kph\n"
                         "        start_byte: 0\n"
                         "        length_bytes: 2\n"
                         "  - name: PowerStatus\n"
                         "    id: 0x102\n"
                         "    cycle_ms: 500\n"
                         "    signals:\n"
                         "      - name: fuel_percent\n"
                         "        start_byte: 0\n"
                         "        length_bytes: 1\n")) {
        return false;
    }

    const std::vector<cansim::CanMessageSchedule> schedule = cansim::loadCanMessageSchedule(path);
    std::filesystem::remove(path);

    bool passed = true;
    passed &= expectEqual("schedule count", schedule.size(), 2);
    passed &= expectEqual("first CAN ID", schedule[0].can_id, 0x100);
    passed &= expectEqual("first cycle", schedule[0].cycle, 100ms);
    passed &= expectEqual("second CAN ID", schedule[1].can_id, 0x102);
    passed &= expectEqual("second cycle", schedule[1].cycle, 500ms);
    return passed;
}

bool testLoadsSignalPackingDefinitions() {
    const std::filesystem::path path = testFilePath("cansim_signal_map_signals.yaml");
    if (!writeFile(path, "messages:\n"
                         "  - name: VehicleMotion\n"
                         "    id: 0x100\n"
                         "    cycle_ms: 100\n"
                         "    signals:\n"
                         "      - name: speed_kph\n"
                         "        start_byte: 0\n"
                         "        length_bytes: 2\n"
                         "        endian: little\n"
                         "        scale: 0.01\n"
                         "        offset: 0\n"
                         "        minimum: 0\n"
                         "        maximum: 655.35\n")) {
        return false;
    }

    const cansim::CanSignalMap signal_map = cansim::loadCanSignalMap(path);
    const std::vector<cansim::CanMessageSchedule> schedule = cansim::messageSchedule(signal_map);
    std::filesystem::remove(path);

    bool passed = true;
    passed &= expectEqual("message count", signal_map.messages.size(), 1);
    passed &= expectString("message name", signal_map.messages[0].name, "VehicleMotion");
    passed &= expectEqual("message CAN ID", signal_map.messages[0].can_id, 0x100);
    passed &= expectEqual("message cycle", signal_map.messages[0].cycle, 100ms);
    passed &= expectEqual("signal count", signal_map.messages[0].signals.size(), 1);
    passed &= expectString("signal name", signal_map.messages[0].signals[0].name, "speed_kph");
    passed &= expectInt("signal start byte", signal_map.messages[0].signals[0].start_byte, 0);
    passed &= expectInt("signal length", signal_map.messages[0].signals[0].length_bytes, 2);
    passed &= expectEndian("signal endian", signal_map.messages[0].signals[0].endian,
                           cansim::CanSignalEndian::little);
    passed &= expectDouble("signal scale", signal_map.messages[0].signals[0].scale, 0.01);
    passed &= expectDouble("signal offset", signal_map.messages[0].signals[0].offset, 0.0);
    passed &= expectEqual("schedule count from map", schedule.size(), 1);
    passed &= expectEqual("schedule CAN ID from map", schedule[0].can_id, 0x100);
    return passed;
}

bool testRejectsMissingCycle() {
    const std::filesystem::path path = testFilePath("cansim_signal_map_missing_cycle.yaml");
    if (!writeFile(path, "messages:\n"
                         "  - name: VehicleMotion\n"
                         "    id: 0x100\n")) {
        return false;
    }

    try {
        (void)cansim::loadCanMessageSchedule(path);
    } catch (const std::runtime_error&) {
        std::filesystem::remove(path);
        return true;
    }

    std::filesystem::remove(path);
    std::cerr << "missing cycle expected exception\n";
    return false;
}

bool testRejectsDuplicateIds() {
    const std::filesystem::path path = testFilePath("cansim_signal_map_duplicate_ids.yaml");
    if (!writeFile(path, "messages:\n"
                         "  - name: VehicleMotion\n"
                         "    id: 0x100\n"
                         "    cycle_ms: 100\n"
                         "    signals:\n"
                         "      - name: speed_kph\n"
                         "        start_byte: 0\n"
                         "        length_bytes: 2\n"
                         "  - name: VehicleMotionAgain\n"
                         "    id: 0x100\n"
                         "    cycle_ms: 200\n"
                         "    signals:\n"
                         "      - name: speed_kph\n"
                         "        start_byte: 0\n"
                         "        length_bytes: 2\n")) {
        return false;
    }

    try {
        (void)cansim::loadCanMessageSchedule(path);
    } catch (const std::runtime_error&) {
        std::filesystem::remove(path);
        return true;
    }

    std::filesystem::remove(path);
    std::cerr << "duplicate ID expected exception\n";
    return false;
}

} // namespace

int main() {
    bool passed = true;
    passed &= testLoadsMessageCycles();
    passed &= testLoadsSignalPackingDefinitions();
    passed &= testRejectsMissingCycle();
    passed &= testRejectsDuplicateIds();

    return passed ? 0 : 1;
}
