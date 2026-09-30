#include "cansim/can_encoder.hpp"
#include "cansim/can_output_factory.hpp"
#include "cansim/cli_options.hpp"

#include <exception>
#include <iostream>
#include <memory>

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
    std::unique_ptr<cansim::CanOutput> output;
    try {
        output = cansim::createCanOutput(parse_result.options, std::cout);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    if (!output) {
        std::cerr << "Selected CAN output is not supported on this platform\n";
        return 1;
    }

    for (int i = 0; i < parse_result.options.frames; ++i) {
        for (const cansim::CanFrame& frame : encoder.encodeAll(state)) {
            output->send(frame);
        }
    }

    return 0;
}
