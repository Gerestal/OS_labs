add_library(Receiver STATIC
    Receiver.cpp
)

target_include_directories(Receiver
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
