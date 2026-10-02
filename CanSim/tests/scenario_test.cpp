#include "cansim/scenario.hpp"

#include <cmath>
#include <iostream>

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

bool expectEqual(const char* name, int actual, int expected) {
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

} // namespace

int main() {
    bool passed = true;

    passed &= expectTrue("idle parses", cansim::parseScenario("idle").has_value());
    passed &= expectTrue("cruise parses", cansim::parseScenario("cruise").has_value());
    passed &= expectTrue("braking parses", cansim::parseScenario("braking").has_value());
    passed &= expectFalse("unknown scenario fails", cansim::parseScenario("launch").has_value());

    const cansim::VehicleState idle = cansim::vehicleStateForScenario(cansim::Scenario::idle);
    passed &= expectDouble("idle speed", idle.speed_kph, 0.0);
    passed &= expectEqual("idle rpm", idle.engine_rpm, 800);
    passed &= expectFalse("idle brake", idle.brake_active);

    const cansim::VehicleState cruise = cansim::vehicleStateForScenario(cansim::Scenario::cruise);
    passed &= expectDouble("cruise speed", cruise.speed_kph, 100.0);
    passed &= expectDouble("cruise throttle", cruise.throttle_percent, 35.0);
    passed &= expectEqual("cruise gear", cruise.gear, 5);

    const cansim::VehicleState braking = cansim::vehicleStateForScenario(cansim::Scenario::braking);
    passed &= expectDouble("braking speed", braking.speed_kph, 40.0);
    passed &= expectDouble("braking throttle", braking.throttle_percent, 0.0);
    passed &= expectTrue("braking brake", braking.brake_active);

    return passed ? 0 : 1;
}
