# Note that it must be before the first project of your top-level CMakeLists.txt, since it needs to be set globally at generation time 
set( CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION_MAXIMUM true)	# Unfortunatly, does not stop build when version is not found
#set( CMAKE_SYSTEM_VERSION 10.0.17134.0)					# This one works
#set( CMAKE_SYSTEM_VERSION "10.0.18362.0")					# This one is buggy
set( CMAKE_SYSTEM_VERSION "10.0.19041.0")					# This one works allso
message("CMAKE_SYSTEM_VERSION " ${CMAKE_SYSTEM_VERSION})
message("CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION " ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION})
message("CMAKE_BINARY_DIR:" ${CMAKE_BINARY_DIR} )
#set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )

# TODO add executable based what system is configured (Linux/Win), should be named the same (?)
add_executable (stylusProxyWin "src/main.cpp" "src/SocketServer.cpp" "src/BLEdeviceFinder.cpp")

# TODO: Add tests and install targets if needed.
set_property(TARGET stylusProxyWin PROPERTY CXX_STANDARD 17)
set_target_properties(stylusProxyWin PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "/dev/stylus1")
target_link_libraries(stylusProxyWin WindowsApp.lib Ws2_32.dll)
message("VS_DEBUGGER_COMMAND_ARGUMENTS " ${VS_DEBUGGER_COMMAND_ARGUMENTS})