#include "cansim/can_encoder.hpp"
#include "cansim/can_output_factory.hpp"
#include "cansim/can_scheduler.hpp"
#include "cansim/can_signal_map.hpp"
#include "cansim/cli_options.hpp"

#include <chrono>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace {

[[nodiscard]] std::filesystem::path resolveSignalMapPath(const std::string& signal_map_path) {
    const std::filesystem::path requested_path{signal_map_path};
    if (std::filesystem::exists(requested_path) || signal_map_path != "config/can-signals.yaml") {
        return requested_path;
    }

    const std::filesystem::path root_layout_path{"CanSim/config/can-signals.yaml"};
    if (std::filesystem::exists(root_layout_path)) {
        return root_layout_path;
    }

    return requested_path;
}

void sendFrames(const std::vector<cansim::CanFrame>& frames, cansim::CanOutput& output) {
    for (const cansim::CanFrame& frame : frames) {
        output.send(frame);
    }
}

[[nodiscard]] std::string formatCanId(std::uint32_t can_id) {
    std::ostringstream stream;
    stream << "0x" << std::hex << std::uppercase << can_id;
    return stream.str();
}

[[nodiscard]] std::vector<cansim::CanMessageSchedule>
fixedScheduleForFrames(const std::vector<cansim::CanFrame>& frames, int period_ms) {
    std::vector<cansim::CanMessageSchedule> schedule;
    schedule.reserve(frames.size());

    for (const cansim::CanFrame& frame : frames) {
        schedule.push_back(cansim::CanMessageSchedule{
            .can_id = frame.id,
            .cycle = std::chrono::milliseconds{period_ms},
        });
    }

    return schedule;
}

[[nodiscard]] std::unordered_map<std::uint32_t, cansim::CanFrame>
indexFramesById(const std::vector<cansim::CanFrame>& frames) {
    std::unordered_map<std::uint32_t, cansim::CanFrame> frames_by_id;
    frames_by_id.reserve(frames.size());

    for (const cansim::CanFrame& frame : frames) {
        frames_by_id.emplace(frame.id, frame);
    }

    return frames_by_id;
}

void sendScheduledFrames(const cansim::CliOptions& options, const cansim::CanSignalMap& signal_map,
                         const std::vector<cansim::CanFrame>& frames, cansim::CanOutput& output) {
    using Clock = std::chrono::steady_clock;

    std::vector<cansim::CanMessageSchedule> schedule =
        options.period_ms.has_value() ? fixedScheduleForFrames(frames, *options.period_ms)
                                      : cansim::messageSchedule(signal_map);

    cansim::CanScheduler scheduler{schedule};
    const std::unordered_map<std::uint32_t, cansim::CanFrame> frames_by_id =
        indexFramesById(frames);

    auto run_duration = std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<double>{*options.duration_seconds});
    if (run_duration <= Clock::duration::zero()) {
        run_duration = Clock::duration{1};
    }

    const auto start_time = Clock::now();
    const auto end_time = start_time + run_duration;

    while (Clock::now() < end_time) {
        const auto now = Clock::now();
        const auto elapsed_time =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);

        for (const std::uint32_t can_id : scheduler.dueMessageIds(elapsed_time)) {
            const auto frame = frames_by_id.find(can_id);
            if (frame == frames_by_id.end()) {
                throw std::runtime_error("CAN signal map contains unimplemented CAN ID " +
                                         formatCanId(can_id));
            }

            output.send(frame->second);
        }

        const std::chrono::milliseconds sleep_duration = scheduler.timeUntilNextDue(elapsed_time);
        if (sleep_duration <= std::chrono::milliseconds::zero()) {
            continue;
        }

        const auto before_sleep = Clock::now();
        if (before_sleep >= end_time) {
            break;
        }

        const auto next_wake_time = before_sleep + sleep_duration;
        std::this_thread::sleep_until(next_wake_time < end_time ? next_wake_time : end_time);
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
    cansim::CanSignalMap signal_map;
    std::vector<cansim::CanFrame> frames;
    try {
        signal_map =
            cansim::loadCanSignalMap(resolveSignalMapPath(parse_result.options.signal_map_path));
        const cansim::SignalMapCanEncoder encoder{signal_map};
        frames = encoder.encodeAll(state);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

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

    try {
        if (parse_result.options.duration_seconds.has_value()) {
            sendScheduledFrames(parse_result.options, signal_map, frames, *output);
            return 0;
        }

        for (int i = 0; i < parse_result.options.frames; ++i) {
            sendFrames(frames, *output);
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    return 0;
}
