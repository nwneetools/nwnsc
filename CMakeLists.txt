cmake_minimum_required(VERSION 2.8)
project(nwn-tools)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_CXX_STANDARD OR CMAKE_CXX_STANDARD LESS 11)
    set(CMAKE_CXX_STANDARD 11)
endif()

# Appends the cmake/modules path to MAKE_MODULE_PATH variable.
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

#
# Make a version file containing the current version from git.
#
include(GetGitRevisionDescription)
git_describe(VERSION --tags --dirty=-dirty)

#parse the version information into pieces.
string(REGEX REPLACE "^v([0-9]+)\\..*" "\\1" VERSION_MAJOR "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" VERSION_MINOR "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" VERSION_PATCH "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+(.*)" "\\1" VERSION_SHA1 "${VERSION}")
set(VERSION_SHORT "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/version.cpp.in
		${CMAKE_CURRENT_BINARY_DIR}/_NwnUtilLib/version.cpp)
set(version_file "${CMAKE_CURRENT_BINARY_DIR}/_NwnUtilLib/version.cpp")

if (APPLE)
    set(CMAKE_MACOSX_RPATH 1)
endif()

find_package(BISON)
if (NOT BISON_FOUND)
	message(FATAL_ERROR "Cannot find bison, need bison installed to generate Parser/Lexer")
endif (NOT BISON_FOUND)

if (UNIX OR APPLE)
	#set(CMAKE_CXX_FLAGS "-Wall -Wno-unused -Wno-write-strings -Wno-deprecated -std=c++11")
	#set(CMAKE_CXX_FLAGS " -w -O2 -m32 -std=c++11")
	set(CMAKE_CXX_FLAGS "-g -w -m32 -std=c++11")
	#set(CMAKE_CXX_FLAGS "-w /EHsc")
endif()

if (MINGW OR CYGWIN)
    #set(CMAKE_CXX_FLAGS "-Wall -Wno-unused -Wno-write-strings -Wno-deprecated -std=c++11 -static")
    set(CMAKE_CXX_FLAGS " -w -O2 -m32 -std=c++11 -static")
    #set(CMAKE_CXX_FLAGS "-g -w -m32 -std=c++11 -static")
    #set(CMAKE_CXX_FLAGS "-w /EHsc")
endif()

if (MSVC)
	foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
		if(${flag_var} MATCHES "/MD")
			string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
		endif(${flag_var} MATCHES "/MD")
	endforeach(flag_var)
endif()

if (BISON_VERSION VERSION_GREATER "3.0.0")
   add_definitions(-DNWN_BISON_3=1)
endif (BISON_VERSION VERSION_GREATER "3.0.0")

#add_subdirectory(_docopt)
#add_subdirectory(_MathLib)
#add_subdirectory(_NmcLib)
add_subdirectory(_NwnBaseLib)
add_subdirectory(_NwnDataLib)
#add_subdirectory(_NwnScriptLib)
add_subdirectory(_NwnUtilLib)
add_subdirectory(_NscLib)
add_subdirectory(nwnsc)
#add_subdirectory(nwnmdl)
