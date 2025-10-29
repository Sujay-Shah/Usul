project (Editor)

file(GLOB SOURCES
    "src/*.cpp"
    "src/*.h"
    "src/**/*.cpp"
    "src/**/*/*.cpp"
)

add_executable(Editor ${SOURCES})

target_include_directories(Editor
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Engine/src/Engine
        ${CMAKE_SOURCE_DIR}/Engine/src/
)

target_link_libraries(Editor 
    PRIVATE 
        Engine
)