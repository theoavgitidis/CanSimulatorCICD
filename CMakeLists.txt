cmake_minimum_required(VERSION 3.20)

project(CanSimulatorCICD VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(cansim_core
    src/can_encoder.cpp
)

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
add_test(NAME cansim_cli_smoke COMMAND cansim --speed 100 --throttle 50 --rpm 3000 --engine-temp 90 --fuel 75 --battery 12.6 --frames 1)

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
