project( Engine )

set( GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE )

# for clion, to avoid cast from 'char*' to 'int' loses precision being treated as error
set(CMAKE_CXX_FLAGS "-fpermissive")

# add sub directories for projects with cmake support (currently glad, spdlog, GLFW)

add_subdirectory( vendor/glad )
add_subdirectory( vendor/spdlog )
add_subdirectory( vendor/glfw )
add_subdirectory( vendor/assimp )

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DENGINE_DEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DENGINE_DEBUG")

# source code
file(GLOB 
    SOURCES CONFIGURE_DEPENDS
		/*.cpp
		/*.h
        src/Engine/*.cpp 
        src/Engine/Core/*.cpp
		src/Engine/Renderer/*.cpp
		src/Engine/Renderer/Camera/*.cpp
        src/Engine/ImGui/*.cpp 
        src/Platform/OpenGL/*.cpp 
        src/Platform/GLFW/*.cpp 

        # stb_image
        vendor/stb_image/*.cpp

        # must compile and link imgui directly to project
        vendor/imgui/*.cpp
)
message("sources : ${SOURCES}")
# library creation

add_library( Engine 
    STATIC 
        ${SOURCES}
		src/Engine/Renderer/FrameBuffer.cpp
		src/Engine/Renderer/FrameBuffer.h
		src/Platform/OpenGL/FrameBufferOpenGL.cpp
		src/Platform/OpenGL/FrameBufferOpenGL.h
		EnginePCH.cpp
)
target_link_libraries( Engine  
    PUBLIC
        GLAD
        glfw
        spdlog
		assimp
)
target_precompile_headers( Engine 
    PUBLIC 
        EnginePCH.h
)
target_include_directories( Engine 
    PUBLIC 
        src 
        src/Engine 
        src/Platform

        vendor/imgui
        vendor/glm

        vendor/spdlog/include
    PRIVATE
        vendor/glfw/include
        vendor/glad/include
        vendor/stb_image
		vendor/assimp/include
)