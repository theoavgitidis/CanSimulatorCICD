#pragma once

#include "cansim/can_scheduler.hpp"

#include <filesystem>
#include <vector>

namespace cansim {

[[nodiscard]] std::vector<CanMessageSchedule>
loadCanMessageSchedule(const std::filesystem::path& signal_map_path);

} // namespace cansim
