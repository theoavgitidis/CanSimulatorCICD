cmake_minimum_required(VERSION 3.20)

project(CanDisplay VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CANDISPLAY_FORMAT_FILES
    include/candisplay/can_decoder.hpp
    include/candisplay/telemetry_state.hpp
    src/can_decoder.cpp
    tests/can_decoder_test.cpp
)

add_library(candisplay_core
    src/can_decoder.cpp
)

target_include_directories(candisplay_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(candisplay_decoder_tests
    tests/can_decoder_test.cpp
)

target_link_libraries(candisplay_decoder_tests
    PRIVATE
        candisplay_core
)

add_test(NAME candisplay_decoder_tests COMMAND candisplay_decoder_tests)

find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format)
if(CLANG_FORMAT_EXECUTABLE)
    add_custom_target(candisplay_format
        COMMAND "${CLANG_FORMAT_EXECUTABLE}" -i ${CANDISPLAY_FORMAT_FILES}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Formatting CanDisplay C++ sources"
    )

    add_test(NAME candisplay_format_check
        COMMAND "${CLANG_FORMAT_EXECUTABLE}" --dry-run --Werror ${CANDISPLAY_FORMAT_FILES}
    )
    set_tests_properties(candisplay_format_check PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
endif()

function(candisplay_enable_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

candisplay_enable_warnings(candisplay_core)
candisplay_enable_warnings(candisplay_decoder_tests)
