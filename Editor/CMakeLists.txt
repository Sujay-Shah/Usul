project (Editor)

file(GLOB SOURCES
    "src/*.cpp"
    "src/**/*.cpp"
    "src/**/*/*.cpp"
)

add_executable(Editor ${SOURCES}
        src/MaterialExample.cpp
        src/MaterialExample.h
        src/VulkanExample.cpp
        src/VulkanExample.h
        src/ModelExample.cpp
        src/ModelExample.h
)

target_link_libraries(Editor 
    PRIVATE 
        Engine
)