#pragma once

#include "cansim/can_output.hpp"
#include "cansim/cli_options.hpp"

#include <memory>
#include <ostream>

namespace cansim {

[[nodiscard]] std::unique_ptr<CanOutput> createCanOutput(const CliOptions& options,
                                                         std::ostream& console_output);

} // namespace cansim
