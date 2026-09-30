#include "cansim/can_encoder.hpp"
#include "cansim/cli_options.hpp"

#include <cstddef>
#include <iomanip>
#include <iostream>

namespace {

void printFrame(const cansim::CanFrame& frame) {
    std::cout << "0x" << std::hex << std::uppercase << frame.id << " [" << std::dec
              << static_cast<int>(frame.dlc) << "]";

    for (std::size_t i = 0; i < frame.dlc; ++i) {
        std::cout << ' ' << std::hex << std::uppercase << std::setw(2)
                  << std::setfill('0') << static_cast<int>(frame.data[i]);
    }

    std::cout << '\n';
}

}  // namespace

int main(int argc, char* argv[]) {
    const cansim::CliParseResult parse_result = cansim::parseCliOptions(argc, argv);

    if (parse_result.status == cansim::CliParseStatus::help) {
        std::cout << cansim::cliUsage();
        return 0;
    }

    if (parse_result.status == cansim::CliParseStatus::error) {
        std::cerr << parse_result.message << '\n';
        std::cerr << cansim::cliUsage();
        return 1;
    }

    const cansim::VehicleState state = cansim::toVehicleState(parse_result.options);
    const cansim::CanEncoder encoder;

    for (int i = 0; i < parse_result.options.frames; ++i) {
        printFrame(encoder.encodeVehicleMotion(state));
        printFrame(encoder.encodeEngineStatus(state));
        printFrame(encoder.encodePowerStatus(state));
        printFrame(encoder.encodeGearStatus(state));
        printFrame(encoder.encodeDiagnostics(state));
    }

    return 0;
}
