#include "cansim/can_signal_map.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
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

bool expectEqual(const char* name, std::chrono::milliseconds actual,
                 std::chrono::milliseconds expected) {
    if (actual == expected) {
        return true;
    }

    std::cerr << name << " expected " << expected.count() << "ms but got " << actual.count()
              << "ms\n";
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
                         "        id: 0x999\n"
                         "        cycle_ms: 1\n"
                         "  - name: PowerStatus\n"
                         "    id: 0x102\n"
                         "    cycle_ms: 500\n")) {
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
                         "  - name: VehicleMotionAgain\n"
                         "    id: 0x100\n"
                         "    cycle_ms: 200\n")) {
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
    passed &= testRejectsMissingCycle();
    passed &= testRejectsDuplicateIds();

    return passed ? 0 : 1;
}
