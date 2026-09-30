#include "cansim/can_output_factory.hpp"

#include "cansim/console_can_output.hpp"
#include "cansim/socket_can_output.hpp"

namespace cansim {

std::unique_ptr<CanOutput> createCanOutput(const CliOptions& options,
                                           std::ostream& console_output) {
    switch (options.output_mode) {
    case OutputMode::console:
        return std::make_unique<ConsoleCanOutput>(console_output);
    case OutputMode::socketcan:
#if defined(__linux__)
        return std::make_unique<SocketCanOutput>(options.interface_name);
#else
        return nullptr;
#endif
    }

    return nullptr;
}

} // namespace cansim
