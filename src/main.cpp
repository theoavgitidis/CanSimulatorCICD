#include "cansim/can_encoder.hpp"
#include "cansim/cli_options.hpp"
#include "cansim/console_can_output.hpp"

#include <iostream>

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
    cansim::ConsoleCanOutput output{std::cout};

    if (parse_result.options.output_mode != cansim::OutputMode::console) {
        std::cerr << "Unsupported output mode\n";
        return 1;
    }

    for (int i = 0; i < parse_result.options.frames; ++i) {
        for (const cansim::CanFrame& frame : encoder.encodeAll(state)) {
            output.send(frame);
        }
    }

    return 0;
}
