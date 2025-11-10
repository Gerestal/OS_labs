cmake_minimum_required(VERSION 3.12)
project(ThreadFunctionsTest)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0600)
endif()


add_library(Functions
    Functions.cpp
    Header.h
)

target_include_directories(Functions PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    target_link_libraries(Functions kernel32)
endif()


add_executable(tests
    Tests.cpp
)

target_link_libraries(tests Functions)

target_include_directories(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_target(test
    COMMAND run_tests
    DEPENDS run_tests
    COMMENT "Running unit tests..."
)

message(STATUS "Project configured successfully!")
message(STATUS "To build: cmake --build .")
message(STATUS "To run tests: cmake --build . --target test OR ./run_tests")
