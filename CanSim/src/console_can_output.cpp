#include "cansim/console_can_output.hpp"

#include <cstddef>
#include <iomanip>

namespace cansim {

ConsoleCanOutput::ConsoleCanOutput(std::ostream& output) : output_{output} {}

void ConsoleCanOutput::send(const CanFrame& frame) {
    output_ << "0x" << std::hex << std::uppercase << frame.id << " [" << std::dec
            << static_cast<int>(frame.dlc) << "]";

    for (std::size_t i = 0; i < frame.dlc; ++i) {
        output_ << ' ' << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                << static_cast<int>(frame.data[i]);
    }

    output_ << '\n';
}

} // namespace cansim
