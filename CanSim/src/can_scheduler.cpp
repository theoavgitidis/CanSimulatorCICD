#include "cansim/can_scheduler.hpp"

#include <algorithm>
#include <stdexcept>

namespace cansim {
namespace {

[[nodiscard]] std::chrono::milliseconds normalizeElapsed(std::chrono::milliseconds elapsed_time) {
    return std::max(elapsed_time, std::chrono::milliseconds::zero());
}

} // namespace

CanScheduler::CanScheduler(const std::vector<CanMessageSchedule>& messages) {
    if (messages.empty()) {
        throw std::invalid_argument("CAN scheduler requires at least one message");
    }

    messages_.reserve(messages.size());
    for (const CanMessageSchedule& message : messages) {
        if (message.cycle <= std::chrono::milliseconds::zero()) {
            throw std::invalid_argument("CAN scheduler message cycles must be positive");
        }

        messages_.push_back(ScheduledMessage{
            .can_id = message.can_id,
            .cycle = message.cycle,
            .next_due = std::chrono::milliseconds::zero(),
        });
    }
}

std::vector<std::uint32_t> CanScheduler::dueMessageIds(std::chrono::milliseconds elapsed_time) {
    const std::chrono::milliseconds normalized_elapsed = normalizeElapsed(elapsed_time);
    std::vector<std::uint32_t> due_ids;

    for (ScheduledMessage& message : messages_) {
        if (normalized_elapsed < message.next_due) {
            continue;
        }

        due_ids.push_back(message.can_id);

        do {
            message.next_due += message.cycle;
        } while (message.next_due <= normalized_elapsed);
    }

    return due_ids;
}

std::chrono::milliseconds
CanScheduler::timeUntilNextDue(std::chrono::milliseconds elapsed_time) const {
    const std::chrono::milliseconds normalized_elapsed = normalizeElapsed(elapsed_time);
    std::chrono::milliseconds next_delay = std::chrono::milliseconds::max();

    for (const ScheduledMessage& message : messages_) {
        const std::chrono::milliseconds delay = message.next_due > normalized_elapsed
                                                    ? message.next_due - normalized_elapsed
                                                    : std::chrono::milliseconds::zero();
        next_delay = std::min(next_delay, delay);
    }

    return next_delay;
}

} // namespace cansim
