project(main_2 LANGUAGES CXX)

add_executable(main_2 main_standard.cpp)

target_include_directories(main_2
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

