#include "cansim/can_encoder.hpp"
#include "cansim/console_can_output.hpp"
#include "cansim/recording_can_output.hpp"
#include "cansim/vehicle_state.hpp"

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

bool expectEqual(const char* name, std::uint32_t actual, std::uint32_t expected) {
    if (actual == expected) {
        return true;
    }

    std::cerr << name << " expected " << expected << " but got " << actual << '\n';
    return false;
}

bool expectEqual(const char* name, std::size_t actual, std::size_t expected) {
    if (actual == expected) {
        return true;
    }

    std::cerr << name << " expected " << expected << " but got " << actual << '\n';
    return false;
}

bool expectString(const char* name, const std::string& actual, const std::string& expected) {
    if (actual == expected) {
        return true;
    }

    std::cerr << name << " expected:\n" << expected << "but got:\n" << actual;
    return false;
}

bool testRecordingOutputStoresEncodedFrames() {
    const cansim::VehicleState state{
        .speed_kph = 100.0,
        .throttle_percent = 50.0,
        .brake_active = false,
        .engine_rpm = 3000,
        .engine_temp_c = 90.0,
        .fuel_percent = 75.0,
        .battery_voltage = 12.6,
        .gear = 3,
        .active_dtc = 0x0420,
    };

    const cansim::CanEncoder encoder;
    cansim::RecordingCanOutput output;

    for (const cansim::CanFrame& frame : encoder.encodeAll(state)) {
        output.send(frame);
    }

    const std::vector<cansim::CanFrame>& frames = output.frames();

    bool passed = true;
    passed &= expectEqual("recorded frame count", frames.size(), 5);
    passed &= expectEqual("recorded frame 0 ID", frames[0].id, 0x100);
    passed &= expectEqual("recorded frame 1 ID", frames[1].id, 0x101);
    passed &= expectEqual("recorded frame 2 ID", frames[2].id, 0x102);
    passed &= expectEqual("recorded frame 3 ID", frames[3].id, 0x103);
    passed &= expectEqual("recorded frame 4 ID", frames[4].id, 0x700);
    return passed;
}

bool testConsoleOutputFormatsFrame() {
    const cansim::CanFrame frame{
        .id = 0x100,
        .data = {0x10, 0x27, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00},
        .dlc = 8,
    };

    std::ostringstream stream;
    cansim::ConsoleCanOutput output{stream};
    output.send(frame);

    return expectString("console frame", stream.str(), "0x100 [8] 10 27 32 00 00 00 00 00\n");
}

} // namespace

int main() {
    bool passed = true;
    passed &= testRecordingOutputStoresEncodedFrames();
    passed &= testConsoleOutputFormatsFrame();

    return passed ? 0 : 1;
}
