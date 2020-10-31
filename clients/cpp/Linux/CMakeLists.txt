#
#  GattLib - GATT Library
#
#  Copyright (C) 2016-2017  Olivier Martin <olivier@labapart.org>
#
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
project(stylus_client) 

cmake_minimum_required(VERSION 2.6)

find_package(PkgConfig REQUIRED)

pkg_search_module(PCRE REQUIRED libpcre)
pkg_search_module(GATTLIB REQUIRED gattlib)
pkg_search_module(GLIB REQUIRED glib-2.0)

include_directories(${GLIB_INCLUDE_DIRS}) 

# Define 'Debug' as the default build type
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug")
endif() 

# Show all the warnings
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

set(stylus_client_SRCS stylus_client.cpp)
set(scan_SRCS scan.c)

add_executable(scan ${scan_SRCS})
add_executable(stylus_client ${stylus_client_SRCS})
add_executable(socket_client socket_client.c)
target_link_libraries(scan ${GATTLIB_LIBRARIES} ${GATTLIB_LDFLAGS} ${PCRE_LIBRARIES} pthread)
target_link_libraries(stylus_client ${GATTLIB_LIBRARIES} ${GATTLIB_LDFLAGS} ${PCRE_LIBRARIES} pthread)