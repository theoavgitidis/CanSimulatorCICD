#include "cansim/can_encoder.hpp"
#include "cansim/can_output_factory.hpp"
#include "cansim/cli_options.hpp"

#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <thread>

namespace {

void sendFrameBurst(const cansim::CanEncoder& encoder, const cansim::VehicleState& state,
                    cansim::CanOutput& output) {
    for (const cansim::CanFrame& frame : encoder.encodeAll(state)) {
        output.send(frame);
    }
}

} // namespace

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

    if (parse_result.options.duration_seconds.has_value()) {
        using Clock = std::chrono::steady_clock;

        auto run_duration = std::chrono::duration_cast<Clock::duration>(
            std::chrono::duration<double>{*parse_result.options.duration_seconds});
        if (run_duration <= Clock::duration::zero()) {
            run_duration = Clock::duration{1};
        }

        const auto period = std::chrono::milliseconds{parse_result.options.period_ms};
        const auto end_time = Clock::now() + run_duration;

        while (Clock::now() < end_time) {
            sendFrameBurst(encoder, state, *output);

            const auto now = Clock::now();
            if (now >= end_time) {
                break;
            }

            const auto next_send_time = now + period;
            std::this_thread::sleep_until(next_send_time < end_time ? next_send_time : end_time);
        }

        return 0;
    }

    for (int i = 0; i < parse_result.options.frames; ++i) {
        sendFrameBurst(encoder, state, *output);
    }

    return 0;
}
