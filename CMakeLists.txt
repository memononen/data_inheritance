cmake_minimum_required(VERSION 3.22)

include(FetchContent)

project(array_merge
	VERSION 0.1.0
	DESCRIPTION "Inherited array merge test"
	HOMEPAGE_URL "https://github.com/memononen/array_merge"
	LANGUAGES CXX C
)

message(STATUS "CMake C compiler: ${CMAKE_C_COMPILER_ID}")
message(STATUS "CMake C++ compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CMake system name: ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMake host system processor: ${CMAKE_HOST_SYSTEM_PROCESSOR}")

# static link
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

include(GNUInstallDirs)

add_subdirectory(src)

set_property(TARGET example PROPERTY DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

if(MSVC)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT example)
	set_property(TARGET example PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
endif()

if(APPLE)
	set_target_properties(example PROPERTIES
		XCODE_GENERATE_SCHEME TRUE
		XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
endif()
