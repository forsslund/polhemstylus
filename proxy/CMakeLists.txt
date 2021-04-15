# CMakeList.txt : CMake project for Polhem Stylus Proxy
# 
cmake_minimum_required (VERSION 3.16)
project ("polhemStylusProxy")

message("CMakeList in " ${CMAKE_CURRENT_SOURCE_DIR} )
IF (WIN32)    
# Note that it must be before the first project of your top-level CMakeLists.txt, since it needs to be set globally at generation time 
#set( CMAKE_SYSTEM_VERSION 10.0.17134.0)					# This SDK works
#set( CMAKE_SYSTEM_VERSION "10.0.18362.0")					# This SDK is buggy
set( CMAKE_SYSTEM_VERSION "10.0.19041.0")					# This SDK works allso
message("CMAKE_SYSTEM_VERSION " ${CMAKE_SYSTEM_VERSION})
set( CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION ${CMAKE_SYSTEM_VERSION})
set( CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION_MAXIMUM ${CMAKE_SYSTEM_VERSION})	# Hmmm
message("CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION:" ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION})

ELSE()
    find_package(PkgConfig REQUIRED)
    pkg_search_module(PCRE REQUIRED libpcre)
    pkg_search_module(GATTLIB REQUIRED gattlib)
    pkg_search_module(GLIB REQUIRED glib-2.0) 
    include_directories(${GLIB_INCLUDE_DIRS})  
ENDIF ()

    set( CMAKE_BINARY_DIR ${PROJECT_SOURCE_DIR}/bin/ )
    message("CMAKE_BINARY_DIR:" ${CMAKE_BINARY_DIR} )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE} )
    message("CMAKE_RUNTIME_OUTPUT_DIRECTORY :" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} )


IF (WIN32)
    add_executable (stylusProxy "src/winMain.cpp" "src/SocketServer.cpp" "src/BLEdeviceFinder.cpp")
    set_target_properties(stylusProxy PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "/dev/stylus1")
    target_link_libraries(stylusProxy WindowsApp.lib Ws2_32.dll)
    message("VS_DEBUGGER_COMMAND_ARGUMENTS " ${VS_DEBUGGER_COMMAND_ARGUMENTS})
ELSE()
    add_executable (stylusProxy "src/glibMain.cpp" "src/SocketServer.cpp")
    target_link_libraries(stylusProxy ${GATTLIB_LIBRARIES} ${GATTLIB_LDFLAGS} ${PCRE_LIBRARIES} pthread)
ENDIF()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set_property(TARGET stylusProxy PROPERTY CXX_STANDARD 17)

# TODO: Add tests and install targets if needed.


