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




# --- Source Code ---
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    "src/*.cpp"
    "src/Engine/*.cpp"
    "src/Platform/*.cpp"
    "vendor/stb_image/*.cpp"
    "EnginePCH.cpp"
)

# Add specific ImGui source files needed for GLFW and OpenGL3 backend
list(APPEND ENGINE_SOURCES
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/backends/imgui_impl_glfw.cpp
    vendor/imgui/backends/imgui_impl_opengl3.cpp # Assuming OpenGL3 is used with GLAD
)
# library creation

add_library( Engine 
    STATIC 
        ${ENGINE_SOURCES}
)
# Set the standard C++ exception handling model for MSVC
if(MSVC)
    target_compile_options(Engine PRIVATE /EHsc)
endif()


find_package(Vulkan REQUIRED)
message($ENV{VULKAN_SDK})
message(${Vulkan_INCLUDE_DIRS})
message(${Vulkan_LIBRARIES})

target_link_libraries( Engine  
    PUBLIC
        GLAD
        glfw
        spdlog
		assimp
		${Vulkan_LIBRARIES}
)

target_precompile_headers( Engine 
    PUBLIC 
        EnginePCH.h
)
target_include_directories( Engine 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
        src 
        src/Engine 
        src/Platform

        vendor/imgui
        vendor/glm
        vendor/entt/single_include/entt
        vendor/spdlog/include
		${Vulkan_INCLUDE_DIRS}
    PRIVATE
        vendor/glfw/include
        vendor/glad/include
        vendor/stb_image
		vendor/assimp/include
        

)

# Make sure you have an 'assets' directory in your project root.
set(ASSET_DIRECTORY "${CMAKE_SOURCE_DIR}/assets")
target_compile_definitions(Engine PRIVATE "ASSET_ROOT=\"${ASSET_DIRECTORY}\"")
