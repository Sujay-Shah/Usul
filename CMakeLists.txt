cmake_minimum_required(VERSION 3.10)
project(usul)

set(CMAKE_CXX_STANDARD 14)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG  ${PROJECT_SOURCE_DIR}/bin/debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/bin/release)

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

link_directories(${PROJECT_SOURCE_DIR}/lib)

# ##############################################################################
# Usul
# ##############################################################################

# spdlog add_subdirectory(hazel/vendor/spdlog)
include_directories(usul/vendor/spdlog/include)

# glad
set(GLAD_SRC usul/vendor/glad/src/glad.c)
if (NOT WIN32)
    list(APPEND LIBS dl)
else ()
    list(APPEND LIBS Opengl32)
endif (NOT WIN32)
include_directories(usul/vendor/glad/include)

# glfw
option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
add_subdirectory(usul/vendor/glfw)
include_directories(usul/vendor/glfw/include)
list(APPEND LIBS glfw ${GLFW_LIBRARIES})

# Imgui
include_directories(usul/vendor/imgui)

# glm
include_directories(usul/vendor/glm)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DHZ_DEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DHZ_DEBUG")
include_directories(usul/src)

# stb_image
include_directories(usul/vendor/stb_image)

file(
        GLOB
        US_SRC
        "usul/src/usul/core/*.cpp"
        "usul/src/usul/debug/*.cpp"
        "usul/src/usul/imgui/*.cpp"
        "usul/src/usul/renderer/*.cpp"
        "usul/src/platform/windows/*.cpp"
        "usul/src/platform/opengl/*.cpp"
        "usul/vendor/imgui/*.cpp"
        "usul/vendor/stb_image/*.cpp")
add_library(usul STATIC ${US_SRC} ${GLAD_SRC})
target_link_libraries(usul PUBLIC ${LIBS})

# ##############################################################################
# Sandbox
# ##############################################################################

file(GLOB SANDBOX_SRC
        "sandbox/src/*.cpp")

add_executable(sandbox ${SANDBOX_SRC})
target_link_libraries(sandbox PUBLIC usul ${LIBS})

add_custom_command(TARGET sandbox
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND}
        -E copy_directory
        ${PROJECT_SOURCE_DIR}/sandbox/assets
        $<TARGET_FILE_DIR:sandbox>/assets
        )