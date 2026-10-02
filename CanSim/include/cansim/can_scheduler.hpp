#pragma once

#include <chrono>
#include <cstdint>
#include <vector>

namespace cansim {

struct CanMessageSchedule {
    std::uint32_t can_id{};
    std::chrono::milliseconds cycle{};
};

class CanScheduler {
  public:
    explicit CanScheduler(const std::vector<CanMessageSchedule>& messages);

    [[nodiscard]] std::vector<std::uint32_t> dueMessageIds(std::chrono::milliseconds elapsed_time);
    [[nodiscard]] std::chrono::milliseconds
    timeUntilNextDue(std::chrono::milliseconds elapsed_time) const;

  private:
    struct ScheduledMessage {
        std::uint32_t can_id{};
        std::chrono::milliseconds cycle{};
        std::chrono::milliseconds next_due{};
    };

    std::vector<ScheduledMessage> messages_{};
};

} // namespace cansim
