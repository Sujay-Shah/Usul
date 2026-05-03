project (Example)

add_executable(Example 
    src/Application.cpp 
    src/TriangleDemo.cpp 
    src/TriangleDemo.h)

target_link_libraries(Example 
    PRIVATE 
        Engine
)
