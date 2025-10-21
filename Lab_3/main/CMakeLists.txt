project(main LANGUAGES CXX)

add_executable(main main.cpp)


target_include_directories(main
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

