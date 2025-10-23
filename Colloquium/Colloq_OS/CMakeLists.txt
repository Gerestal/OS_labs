cmake_minimum_required(VERSION 3.10)
project(TasksProject)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(tasks
    task.cpp
    functions.cpp
    Header_functions.h
)

target_include_directories(tasks PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

