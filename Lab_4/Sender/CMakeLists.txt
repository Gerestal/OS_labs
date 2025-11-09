add_library(Creator STATIC
    Sender.cpp
)

target_include_directories(Sender
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
