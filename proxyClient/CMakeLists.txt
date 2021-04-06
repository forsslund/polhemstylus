# TODO add executable based what system is configured (Linux/Win), should be named the same (?)
cmake_minimum_required (VERSION 3.19)
project ("proxyClient")

add_executable (proxyClient "winMain.cpp")

# TODO: Add tests and install targets if needed.
set_property(TARGET proxyClient PROPERTY CXX_STANDARD 17)
target_link_libraries(proxyClient WindowsApp.lib Ws2_32.dll)