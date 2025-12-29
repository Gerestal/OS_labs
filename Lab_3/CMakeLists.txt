cmake_minimum_required(VERSION 3.16)
project(MySolution LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(main)
add_subdirectory(main_2)
add_subdirectory(tests)

add_custom_target(run_all_tests
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_tests
    DEPENDS For_tests
    COMMENT "Running all tests..."
)