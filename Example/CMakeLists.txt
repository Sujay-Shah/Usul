project (Example)

file(GLOB SOURCES
    "src/*.cpp"
    "src/**/*.cpp"
    "src/**/*/*.cpp"
)

add_executable(Example ${SOURCES}
        src/MaterialExample.cpp
        src/MaterialExample.h
        src/VulkanExample.cpp
        src/VulkanExample.h
        src/ModelExample.cpp
        src/ModelExample.h
)

target_link_libraries(Example 
    PRIVATE 
        Engine
)
