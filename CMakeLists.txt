# CMakeList.txt : CMake project for Polhem Stylus integration
#
cmake_minimum_required (VERSION 3.16)

# Note that it must be before the first project of your top-level CMakeLists.txt, since it needs to be set globally at generation time 
set( CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION_MAXIMUM true)	# Unfortunatly, does not stop build when version is not found
#set( CMAKE_SYSTEM_VERSION 10.0.17134.0)					# This one works
#set( CMAKE_SYSTEM_VERSION "10.0.18362.0")					# This one is buggy
set( CMAKE_SYSTEM_VERSION "10.0.19041.0")					# This one works allso
message("CMAKE_SYSTEM_VERSION " ${CMAKE_SYSTEM_VERSION})
message("CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION " ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION})
message("CMAKE_BINARY_DIR:" ${CMAKE_BINARY_DIR} )
#set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )

project ("polhemStylus")

# Recurse into the "Hello" and "Demo" subdirectories. This does not actually
# cause another cmake executable to run. The same process will walk through
# the project's entire directory structure.
add_subdirectory (proxy)
#add_subdirectory (proxyClient)	// commented out since this is a Standalone project
