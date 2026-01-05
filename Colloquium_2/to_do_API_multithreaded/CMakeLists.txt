cmake_minimum_required(VERSION 3.14)
project(TodoListProject VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /EHsc)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -pedantic -g)
endif()

add_definitions(-DSQLITE_THREADSAFE=1 -DSQLITE_OMIT_LOAD_EXTENSION)

add_executable(todo_app
    To_do_list.cpp
    database.cpp
    sqlite3.c
)

add_executable(tests
    test.cpp
    database.cpp
    sqlite3.c
)

target_include_directories(todo_app PRIVATE .)
target_include_directories(tests PRIVATE .)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/catch.hpp)
    message(STATUS "Catch2 found")
endif()

if(WIN32)
    target_link_libraries(todo_app ws2_32 wsock32)
    target_link_libraries(tests ws2_32 wsock32)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(todo_app Threads::Threads)
    target_link_libraries(tests Threads::Threads)
    
    if(UNIX AND NOT APPLE)
        target_link_libraries(todo_app dl)
        target_link_libraries(tests dl)
    endif()
endif()

