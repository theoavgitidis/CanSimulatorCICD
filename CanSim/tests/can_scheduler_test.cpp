#include "cansim/can_scheduler.hpp"

#include <chrono>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

using namespace std::chrono_literals;

bool expectEqual(const char* name, std::chrono::milliseconds actual,
                 std::chrono::milliseconds expected) {
    if (actual == expected) {
        return true;
    }

    std::cerr << name << " expected " << expected.count() << "ms but got " << actual.count()
              << "ms\n";
    return false;
}

bool expectIds(const char* name, const std::vector<std::uint32_t>& actual,
               std::initializer_list<std::uint32_t> expected) {
    if (actual.size() != expected.size()) {
        std::cerr << name << " expected " << expected.size() << " IDs but got " << actual.size()
                  << '\n';
        return false;
    }

    std::size_t index = 0;
    for (const std::uint32_t expected_id : expected) {
        if (actual[index] != expected_id) {
            std::cerr << name << " expected ID " << expected_id << " at index " << index
                      << " but got " << actual[index] << '\n';
            return false;
        }
        ++index;
    }

    return true;
}

bool testInitialTickSendsAllMessages() {
    const std::vector<cansim::CanMessageSchedule> schedule{
        {.can_id = 0x100, .cycle = 100ms},
        {.can_id = 0x102, .cycle = 500ms},
        {.can_id = 0x700, .cycle = 1000ms},
    };
    cansim::CanScheduler scheduler{schedule};

    bool passed = true;
    passed &= expectIds("initial due IDs", scheduler.dueMessageIds(0ms), {0x100, 0x102, 0x700});
    passed &= expectEqual("next due after initial tick", scheduler.timeUntilNextDue(0ms), 100ms);
    return passed;
}

bool testMessagesBecomeDueOnTheirOwnCycles() {
    const std::vector<cansim::CanMessageSchedule> schedule{
        {.can_id = 0x100, .cycle = 100ms},
        {.can_id = 0x102, .cycle = 500ms},
        {.can_id = 0x700, .cycle = 1000ms},
    };
    cansim::CanScheduler scheduler{schedule};

    bool passed = true;
    passed &= expectIds("initial due IDs", scheduler.dueMessageIds(0ms), {0x100, 0x102, 0x700});
    passed &= expectIds("100ms due IDs", scheduler.dueMessageIds(100ms), {0x100});
    passed &= expectIds("200ms due IDs", scheduler.dueMessageIds(200ms), {0x100});
    passed &= expectIds("500ms due IDs", scheduler.dueMessageIds(500ms), {0x100, 0x102});
    passed &= expectIds("1000ms due IDs", scheduler.dueMessageIds(1000ms), {0x100, 0x102, 0x700});
    return passed;
}

bool testLateTickSendsOneDueFrameAndAdvancesPastElapsedTime() {
    const std::vector<cansim::CanMessageSchedule> schedule{
        {.can_id = 0x100, .cycle = 100ms},
    };
    cansim::CanScheduler scheduler{schedule};

    bool passed = true;
    passed &= expectIds("initial due IDs", scheduler.dueMessageIds(0ms), {0x100});
    passed &= expectIds("late due IDs", scheduler.dueMessageIds(350ms), {0x100});
    passed &= expectIds("same late tick due IDs", scheduler.dueMessageIds(350ms), {});
    passed &= expectEqual("next due after late tick", scheduler.timeUntilNextDue(350ms), 50ms);
    return passed;
}

bool testRejectsInvalidSchedules() {
    try {
        const std::vector<cansim::CanMessageSchedule> schedule{
            {.can_id = 0x100, .cycle = 0ms},
        };
        const cansim::CanScheduler scheduler{schedule};
    } catch (const std::invalid_argument&) {
        return true;
    }

    std::cerr << "invalid schedule expected exception\n";
    return false;
}

} // namespace

int main() {
    bool passed = true;
    passed &= testInitialTickSendsAllMessages();
    passed &= testMessagesBecomeDueOnTheirOwnCycles();
    passed &= testLateTickSendsOneDueFrameAndAdvancesPastElapsedTime();
    passed &= testRejectsInvalidSchedules();

    return passed ? 0 : 1;
}
