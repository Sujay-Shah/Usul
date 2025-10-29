cmake_minimum_required( VERSION 3.16 )
set( CMAKE_CXX_STANDARD 17 )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin )
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project( Usul )

add_subdirectory( Engine )
add_subdirectory( Editor )
add_subdirectory( Example )

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Editor)