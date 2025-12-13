cmake_minimum_required( VERSION 3.16 )
project(Usul LANGUAGES CXX OBJCXX)

set( CMAKE_CXX_STANDARD 17 )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin )
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Graphics API Selection
if(WIN32)
    option(ENGINE_USE_VULKAN "Use Vulkan Graphics API" OFF)
    option(ENGINE_USE_DIRECTX "Use DirectX Graphics API" OFF)

    if(ENGINE_USE_VULKAN)
        add_compile_definitions(ENGINE_USE_VULKAN)
    elseif(ENGINE_USE_DIRECTX)
        add_compile_definitions(ENGINE_USE_DIRECTX)
    endif()
elseif(APPLE)
    # macOS uses Vulkan (MoltenVK) by default in PlatformDetection.h
elseif(ANDROID)
    # Android uses Vulkan by default in PlatformDetection.h
elseif(UNIX)
    # Linux
    option(ENGINE_USE_VULKAN "Use Vulkan Graphics API" OFF)
    if(ENGINE_USE_VULKAN)
        add_compile_definitions(ENGINE_USE_VULKAN)
    endif()
endif()

add_subdirectory( Engine )
add_subdirectory( Editor )
add_subdirectory( Example )

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Editor)