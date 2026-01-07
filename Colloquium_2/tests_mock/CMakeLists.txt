cmake_minimum_required(VERSION 3.10)
project(gateway_tests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror -pedantic -pthread)
endif()

add_executable(gateway_tests
    gateway.cpp
    test_gateway.cpp
)

target_include_directories(gateway_tests PRIVATE .)

add_custom_target(run_tests
    COMMAND ${CMAKE_BINARY_DIR}/gateway_tests
    DEPENDS gateway_tests
    COMMENT "Running gateway tests..."
)

add_custom_target(debug_tests
    COMMAND ${CMAKE_BINARY_DIR}/gateway_tests --success
    DEPENDS gateway_tests
    COMMENT "Running gateway tests with details..."
)