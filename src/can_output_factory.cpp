#include "cansim/can_output_factory.hpp"

#include "cansim/console_can_output.hpp"

namespace cansim {

std::unique_ptr<CanOutput> createCanOutput(const CliOptions& options, std::ostream& console_output) {
    switch (options.output_mode) {
        case OutputMode::console:
            return std::make_unique<ConsoleCanOutput>(console_output);
    }

    return nullptr;
}

} // namespace cansim
