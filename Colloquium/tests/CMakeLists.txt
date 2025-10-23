cmake_minimum_required(VERSION 3.12)
project(FunctionsTest)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_executable(run_tests
    test.cpp
    functions.cpp
    Header_functions.h
)

target_include_directories(run_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../ColLog_OS
    ${doctest_SOURCE_DIR}
)

target_compile_definitions(run_tests PRIVATE
    DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
)

if(WIN32)
    target_link_options(run_tests PRIVATE "/SUBSYSTEM:CONSOLE")
endif()

enable_testing()
add_test(NAME AllTests COMMAND run_tests)
