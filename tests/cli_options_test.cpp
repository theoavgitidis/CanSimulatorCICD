#include "cansim/cli_options.hpp"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>

namespace {

bool expectTrue(const char* name, bool value) {
    if (value) {
        return true;
    }

    std::cerr << name << " expected true but got false\n";
    return false;
}

bool expectEqual(const char* name, int actual, int expected) {
    if (actual == expected) {
        return true;
    }

    std::cerr << name << " expected " << expected << " but got " << actual << '\n';
    return false;
}

bool expectEqual(const char* name, std::uint16_t actual, std::uint16_t expected) {
    if (actual == expected) {
        return true;
    }

    std::cerr << name << " expected " << expected << " but got " << actual << '\n';
    return false;
}

bool expectEqual(const char* name, cansim::OutputMode actual, cansim::OutputMode expected) {
    if (actual == expected) {
        return true;
    }

    std::cerr << name << " had unexpected output mode\n";
    return false;
}

bool expectString(const char* name, const std::string& actual, const std::string& expected) {
    if (actual == expected) {
        return true;
    }

    std::cerr << name << " expected " << expected << " but got " << actual << '\n';
    return false;
}

bool expectDouble(const char* name, double actual, double expected) {
    if (std::fabs(actual - expected) < 0.0001) {
        return true;
    }

    std::cerr << name << " expected " << expected << " but got " << actual << '\n';
    return false;
}

bool expectStatus(const char* name, cansim::CliParseStatus actual,
                  cansim::CliParseStatus expected) {
    if (actual == expected) {
        return true;
    }

    std::cerr << name << " had unexpected parser status\n";
    return false;
}

bool testParsesAllOptions() {
    char arg0[] = "cansim";
    char arg1[] = "--speed";
    char arg2[] = "123.45";
    char arg3[] = "--throttle";
    char arg4[] = "67.5";
    char arg5[] = "--brake";
    char arg6[] = "true";
    char arg7[] = "--rpm";
    char arg8[] = "2500";
    char arg9[] = "--engine-temp";
    char arg10[] = "88.5";
    char arg11[] = "--fuel";
    char arg12[] = "42";
    char arg13[] = "--battery";
    char arg14[] = "13.4";
    char arg15[] = "--gear";
    char arg16[] = "-1";
    char arg17[] = "--dtc";
    char arg18[] = "0x0420";
    char arg19[] = "--frames";
    char arg20[] = "3";
    char arg21[] = "--duration";
    char arg22[] = "0.25";
    char arg23[] = "--period-ms";
    char arg24[] = "20";
    char arg25[] = "--signal-map";
    char arg26[] = "custom-signals.yaml";
    char arg27[] = "--output";
    char arg28[] = "console";
    char* argv[] = {arg0,  arg1,  arg2,  arg3,  arg4,  arg5,  arg6,  arg7,  arg8,  arg9,
                    arg10, arg11, arg12, arg13, arg14, arg15, arg16, arg17, arg18, arg19,
                    arg20, arg21, arg22, arg23, arg24, arg25, arg26, arg27, arg28};

    const cansim::CliParseResult result = cansim::parseCliOptions(29, argv);

    bool passed = true;
    passed &= expectStatus("all options status", result.status, cansim::CliParseStatus::ok);
    passed &= expectDouble("speed", result.options.speed_kph, 123.45);
    passed &= expectDouble("throttle", result.options.throttle_percent, 67.5);
    passed &= expectTrue("brake", result.options.brake_active);
    passed &= expectEqual("rpm", result.options.engine_rpm, 2500);
    passed &= expectDouble("engine temperature", result.options.engine_temp_c, 88.5);
    passed &= expectDouble("fuel", result.options.fuel_percent, 42.0);
    passed &= expectDouble("battery", result.options.battery_voltage, 13.4);
    passed &= expectEqual("gear", result.options.gear, -1);
    passed &= expectEqual("dtc", result.options.active_dtc, static_cast<std::uint16_t>(0x0420));
    passed &= expectEqual("frames", result.options.frames, 3);
    passed &= expectTrue("duration present", result.options.duration_seconds.has_value());
    passed &= expectDouble("duration", *result.options.duration_seconds, 0.25);
    passed &= expectTrue("period present", result.options.period_ms.has_value());
    passed &= expectEqual("period", *result.options.period_ms, 20);
    passed &= expectEqual("output mode", result.options.output_mode, cansim::OutputMode::console);
    passed &=
        expectString("signal map path", result.options.signal_map_path, "custom-signals.yaml");
    return passed;
}

bool testParsesSocketCanOutput() {
    char arg0[] = "cansim";
    char arg1[] = "--output";
    char arg2[] = "socketcan";
    char arg3[] = "--interface";
    char arg4[] = "vcan0";
    char* argv[] = {arg0, arg1, arg2, arg3, arg4};

    const cansim::CliParseResult result = cansim::parseCliOptions(5, argv);

    bool passed = true;
    passed &= expectStatus("socketcan output status", result.status, cansim::CliParseStatus::ok);
    passed &= expectEqual("socketcan output mode", result.options.output_mode,
                          cansim::OutputMode::socketcan);
    passed &= expectString("socketcan interface", result.options.interface_name, "vcan0");
    return passed;
}

bool testHelpStatus() {
    char arg0[] = "cansim";
    char arg1[] = "--help";
    char* argv[] = {arg0, arg1};

    const cansim::CliParseResult result = cansim::parseCliOptions(2, argv);
    return expectStatus("help status", result.status, cansim::CliParseStatus::help);
}

bool testScenarioProvidesBaseState() {
    char arg0[] = "cansim";
    char arg1[] = "--scenario";
    char arg2[] = "cruise";
    char* argv[] = {arg0, arg1, arg2};

    const cansim::CliParseResult result = cansim::parseCliOptions(3, argv);

    bool passed = true;
    passed &= expectStatus("scenario status", result.status, cansim::CliParseStatus::ok);
    passed &= expectDouble("scenario speed", result.options.speed_kph, 100.0);
    passed &= expectDouble("scenario throttle", result.options.throttle_percent, 35.0);
    passed &= expectEqual("scenario rpm", result.options.engine_rpm, 2500);
    passed &= expectEqual("scenario gear", result.options.gear, 5);
    return passed;
}

bool testManualValuesOverrideScenario() {
    char arg0[] = "cansim";
    char arg1[] = "--speed";
    char arg2[] = "80";
    char arg3[] = "--scenario";
    char arg4[] = "cruise";
    char arg5[] = "--gear";
    char arg6[] = "4";
    char* argv[] = {arg0, arg1, arg2, arg3, arg4, arg5, arg6};

    const cansim::CliParseResult result = cansim::parseCliOptions(7, argv);

    bool passed = true;
    passed &= expectStatus("scenario override status", result.status, cansim::CliParseStatus::ok);
    passed &= expectDouble("override speed", result.options.speed_kph, 80.0);
    passed &= expectEqual("override gear", result.options.gear, 4);
    passed &= expectEqual("scenario rpm still applies", result.options.engine_rpm, 2500);
    return passed;
}

bool testInvalidScenarioFails() {
    char arg0[] = "cansim";
    char arg1[] = "--scenario";
    char arg2[] = "launch";
    char* argv[] = {arg0, arg1, arg2};

    const cansim::CliParseResult result = cansim::parseCliOptions(3, argv);
    return expectStatus("invalid scenario status", result.status, cansim::CliParseStatus::error);
}

bool testInvalidGearFails() {
    char arg0[] = "cansim";
    char arg1[] = "--gear";
    char arg2[] = "3.7";
    char* argv[] = {arg0, arg1, arg2};

    const cansim::CliParseResult result = cansim::parseCliOptions(3, argv);
    return expectStatus("invalid gear status", result.status, cansim::CliParseStatus::error);
}

bool testInvalidDtcFails() {
    char arg0[] = "cansim";
    char arg1[] = "--dtc";
    char arg2[] = "0x10000";
    char* argv[] = {arg0, arg1, arg2};

    const cansim::CliParseResult result = cansim::parseCliOptions(3, argv);
    return expectStatus("invalid dtc status", result.status, cansim::CliParseStatus::error);
}

bool testInvalidOutputModeFails() {
    char arg0[] = "cansim";
    char arg1[] = "--output";
    char arg2[] = "file";
    char* argv[] = {arg0, arg1, arg2};

    const cansim::CliParseResult result = cansim::parseCliOptions(3, argv);
    return expectStatus("invalid output mode status", result.status, cansim::CliParseStatus::error);
}

bool testInvalidDurationFails() {
    char arg0[] = "cansim";
    char arg1[] = "--duration";
    char arg2[] = "0";
    char* argv[] = {arg0, arg1, arg2};

    const cansim::CliParseResult result = cansim::parseCliOptions(3, argv);
    return expectStatus("invalid duration status", result.status, cansim::CliParseStatus::error);
}

bool testInvalidPeriodFails() {
    char arg0[] = "cansim";
    char arg1[] = "--period-ms";
    char arg2[] = "-1";
    char* argv[] = {arg0, arg1, arg2};

    const cansim::CliParseResult result = cansim::parseCliOptions(3, argv);
    return expectStatus("invalid period status", result.status, cansim::CliParseStatus::error);
}

bool testInvalidSignalMapFails() {
    char arg0[] = "cansim";
    char arg1[] = "--signal-map";
    char arg2[] = "";
    char* argv[] = {arg0, arg1, arg2};

    const cansim::CliParseResult result = cansim::parseCliOptions(3, argv);
    return expectStatus("invalid signal map status", result.status, cansim::CliParseStatus::error);
}

bool testMissingValueFails() {
    char arg0[] = "cansim";
    char arg1[] = "--speed";
    char* argv[] = {arg0, arg1};

    const cansim::CliParseResult result = cansim::parseCliOptions(2, argv);
    return expectStatus("missing value status", result.status, cansim::CliParseStatus::error);
}

bool testStateConversion() {
    cansim::CliOptions options;
    options.speed_kph = 11.0;
    options.throttle_percent = 22.0;
    options.brake_active = true;
    options.engine_rpm = 1500;
    options.engine_temp_c = 77.0;
    options.fuel_percent = 55.0;
    options.battery_voltage = 12.4;
    options.gear = 2;
    options.active_dtc = 0x0007;

    const cansim::VehicleState state = cansim::toVehicleState(options);

    bool passed = true;
    passed &= expectDouble("state speed", state.speed_kph, 11.0);
    passed &= expectDouble("state throttle", state.throttle_percent, 22.0);
    passed &= expectTrue("state brake", state.brake_active);
    passed &= expectEqual("state rpm", state.engine_rpm, 1500);
    passed &= expectDouble("state engine temperature", state.engine_temp_c, 77.0);
    passed &= expectDouble("state fuel", state.fuel_percent, 55.0);
    passed &= expectDouble("state battery", state.battery_voltage, 12.4);
    passed &= expectEqual("state gear", state.gear, 2);
    passed &= expectEqual("state dtc", state.active_dtc, static_cast<std::uint16_t>(0x0007));
    return passed;
}

} // namespace

int main() {
    bool passed = true;
    passed &= testParsesAllOptions();
    passed &= testParsesSocketCanOutput();
    passed &= testHelpStatus();
    passed &= testScenarioProvidesBaseState();
    passed &= testManualValuesOverrideScenario();
    passed &= testInvalidScenarioFails();
    passed &= testInvalidGearFails();
    passed &= testInvalidDtcFails();
    passed &= testInvalidOutputModeFails();
    passed &= testInvalidDurationFails();
    passed &= testInvalidPeriodFails();
    passed &= testInvalidSignalMapFails();
    passed &= testMissingValueFails();
    passed &= testStateConversion();

    return passed ? 0 : 1;
}
