project (Editor)

file(GLOB SOURCES
    "src/*.cpp"
    "src/**/*.cpp"
)

add_executable(Editor ${SOURCES})

target_link_libraries(Editor 
    PRIVATE 
        Engine
)