cmake_minimum_required(VERSION 3.20)

project(CanSimulatorCICD VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

add_subdirectory(CanSim build-cansim)
add_subdirectory(CanDisplay build-candisplay)

set(PROJECT_FORMAT_TARGETS)
if(TARGET cansim_format)
    list(APPEND PROJECT_FORMAT_TARGETS cansim_format)
endif()
if(TARGET candisplay_format)
    list(APPEND PROJECT_FORMAT_TARGETS candisplay_format)
endif()
if(PROJECT_FORMAT_TARGETS)
    add_custom_target(format DEPENDS ${PROJECT_FORMAT_TARGETS})
endif()
