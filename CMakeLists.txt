cmake_minimum_required(VERSION 3.20)

project(CanSimulatorCICD VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CANSIM_FORMAT_FILES
    include/cansim/can_encoder.hpp
    include/cansim/can_frame.hpp
    include/cansim/can_output.hpp
    include/cansim/can_output_factory.hpp
    include/cansim/cli_options.hpp
    include/cansim/console_can_output.hpp
    include/cansim/recording_can_output.hpp
    include/cansim/scenario.hpp
    include/cansim/socket_can_output.hpp
    include/cansim/vehicle_state.hpp
    src/can_encoder.cpp
    src/can_output_factory.cpp
    src/cli_options.cpp
    src/console_can_output.cpp
    src/main.cpp
    src/scenario.cpp
    src/socket_can_output_linux.cpp
    tests/can_encoder_test.cpp
    tests/cli_options_test.cpp
    tests/can_output_factory_test.cpp
    tests/can_output_test.cpp
    tests/scenario_test.cpp
)

add_library(cansim_core
    src/can_encoder.cpp
    src/can_output_factory.cpp
    src/cli_options.cpp
    src/console_can_output.cpp
    src/scenario.cpp
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(cansim_core
        PRIVATE
            src/socket_can_output_linux.cpp
    )
endif()

target_include_directories(cansim_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(cansim
    src/main.cpp
)

target_link_libraries(cansim
    PRIVATE
        cansim_core
)

enable_testing()

add_executable(cansim_tests
    tests/can_encoder_test.cpp
)

target_link_libraries(cansim_tests
    PRIVATE
        cansim_core
)

add_test(NAME cansim_tests COMMAND cansim_tests)

add_executable(cansim_cli_options_tests
    tests/cli_options_test.cpp
)

target_link_libraries(cansim_cli_options_tests
    PRIVATE
        cansim_core
)

add_test(NAME cansim_cli_options_tests COMMAND cansim_cli_options_tests)

add_executable(cansim_scenario_tests
    tests/scenario_test.cpp
)

target_link_libraries(cansim_scenario_tests
    PRIVATE
        cansim_core
)

add_test(NAME cansim_scenario_tests COMMAND cansim_scenario_tests)

add_executable(cansim_output_tests
    tests/can_output_test.cpp
)

target_link_libraries(cansim_output_tests
    PRIVATE
        cansim_core
)

add_test(NAME cansim_output_tests COMMAND cansim_output_tests)

add_executable(cansim_output_factory_tests
    tests/can_output_factory_test.cpp
)

target_link_libraries(cansim_output_factory_tests
    PRIVATE
        cansim_core
)

add_test(NAME cansim_output_factory_tests COMMAND cansim_output_factory_tests)

add_test(NAME cansim_cli_smoke COMMAND cansim --scenario cruise --speed 100 --throttle 50 --rpm 3000 --engine-temp 90 --fuel 75 --battery 12.6 --gear 3 --dtc 0x0420 --frames 1 --output console)

add_test(NAME cansim_cli_duration_smoke COMMAND cansim --scenario cruise --duration 0.002 --period-ms 1 --output console)

find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format)
if(CLANG_FORMAT_EXECUTABLE)
    add_custom_target(format
        COMMAND "${CLANG_FORMAT_EXECUTABLE}" -i ${CANSIM_FORMAT_FILES}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Formatting C++ sources"
    )

    add_test(NAME cansim_format_check
        COMMAND "${CLANG_FORMAT_EXECUTABLE}" --dry-run --Werror ${CANSIM_FORMAT_FILES}
    )
    set_tests_properties(cansim_format_check PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
else()
    message(STATUS "clang-format not found; format target and format check test are disabled")
endif()

function(cansim_enable_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

cansim_enable_warnings(cansim_core)
cansim_enable_warnings(cansim)
cansim_enable_warnings(cansim_tests)
cansim_enable_warnings(cansim_cli_options_tests)
cansim_enable_warnings(cansim_scenario_tests)
cansim_enable_warnings(cansim_output_tests)
cansim_enable_warnings(cansim_output_factory_tests)
