cmake_minimum_required(VERSION 3.15)
project(MySolution LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(Sender)
add_subdirectory(Receiver)
add_subdirectory(tests)

