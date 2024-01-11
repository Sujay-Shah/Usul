project (Editor)

file(GLOB SOURCES
    "src/*.cpp"
    "src/**/*.cpp"
    "src/**/*/*.cpp"
)

add_executable(Editor ${SOURCES}
        src/MaterialExample.cpp
        src/MaterialExample.h
)

target_link_libraries(Editor 
    PRIVATE 
        Engine
)