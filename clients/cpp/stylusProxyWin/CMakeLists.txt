# CMakeList.txt : CMake project for stylusProxyWin, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.19)

# Note that it must be before the first project of your top-level CMakeLists.txt, since it needs to be set globally at generation time 
set( CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION_MAXIMUM true)	# Unfortunatly, does not stop build when version is not found
#set( CMAKE_SYSTEM_VERSION 10.0.17134.0)					# This one works
#set( CMAKE_SYSTEM_VERSION "10.0.18362.0")					# This one is buggy
set( CMAKE_SYSTEM_VERSION "10.0.19041.0")					# This one works allso
message("CMAKE_SYSTEM_VERSION " ${CMAKE_SYSTEM_VERSION})
message("CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION " ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION})
message("CMAKE_BINARY_DIR:" ${CMAKE_BINARY_DIR} )
#set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )

project ("stylusProxyWin")

include_directories( "../common")  

# Add source to this project's executable.
add_executable (stylusProxyWin "main.cpp" "../common/SocketServer.cpp" BLEdeviceFinder.cpp)

# TODO: Add tests and install targets if needed.
set_property(TARGET stylusProxyWin PROPERTY CXX_STANDARD 17)
set_target_properties(stylusProxyWin PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "/dev/stylus1")
#set_property(TARGET stylusProxyWin PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS "/dev/stylus1 786876")
target_link_libraries(stylusProxyWin WindowsApp.lib Ws2_32.dll)