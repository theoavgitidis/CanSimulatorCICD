#include "cansim/can_output_factory.hpp"
#include "cansim/cli_options.hpp"

#include <iostream>
#include <memory>
#include <sstream>
#include <string>

namespace {

bool expectTrue(const char* name, bool value) {
    if (value) {
        return true;
    }

    std::cerr << name << " expected true but got false\n";
    return false;
}

bool expectFalse(const char* name, bool value) {
    if (!value) {
        return true;
    }

    std::cerr << name << " expected false but got true\n";
    return false;
}

bool expectString(const char* name, const std::string& actual, const std::string& expected) {
    if (actual == expected) {
        return true;
    }

    std::cerr << name << " expected:\n" << expected << "but got:\n" << actual;
    return false;
}

} // namespace

int main() {
    cansim::CliOptions options;
    options.output_mode = cansim::OutputMode::console;

    std::ostringstream stream;
    std::unique_ptr<cansim::CanOutput> output = cansim::createCanOutput(options, stream);

    bool passed = true;
    passed &= expectTrue("factory creates console output", static_cast<bool>(output));

    if (output) {
        const cansim::CanFrame frame{
            .id = 0x100,
            .data = {0x10, 0x27, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00},
            .dlc = 8,
        };
        output->send(frame);
    }

    passed &=
        expectString("factory output text", stream.str(), "0x100 [8] 10 27 32 00 00 00 00 00\n");

#if !defined(__linux__)
    options.output_mode = cansim::OutputMode::socketcan;
    options.interface_name = "vcan0";
    std::unique_ptr<cansim::CanOutput> unsupported_output =
        cansim::createCanOutput(options, stream);
    passed &= expectFalse("factory rejects socketcan on non-linux",
                          static_cast<bool>(unsupported_output));
#endif

    return passed ? 0 : 1;
}
