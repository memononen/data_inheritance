cmake_minimum_required(VERSION 3.22)

include(FetchContent)

if(POLICY CMP0135)
	cmake_policy(SET CMP0135 NEW)
endif()

project(data_inheritance
	VERSION 0.1.0
	DESCRIPTION "Data Inheritance Prototype"
	HOMEPAGE_URL "https://github.com/memononen/data_inheritance"
	LANGUAGES CXX C
)

message(STATUS "CMake C compiler: ${CMAKE_C_COMPILER_ID}")
message(STATUS "CMake C++ compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CMake system name: ${CMAKE_SYSTEM_NAME}")

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED YES)
set(CMAKE_C_EXTENSIONS NO)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_UNITY_BUILD OFF)

include(GNUInstallDirs)

# Glad
set(GLAD_DIR ${CMAKE_SOURCE_DIR}/extern/glad)

#SDL3
set(SDL_SHARED OFF)
set(SDL_STATIC ON)
FetchContent_Declare(
	SDL3
	GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
	GIT_TAG release-3.4.0
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(SDL3)

# Dear ImGui
FetchContent_Declare(imgui_external
	GIT_REPOSITORY https://github.com/ocornut/imgui.git
	GIT_TAG v1.92.7-docking
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(imgui_external)

add_library(imgui
	${imgui_external_SOURCE_DIR}/imgui.h
	${imgui_external_SOURCE_DIR}/imgui.cpp
	${imgui_external_SOURCE_DIR}/imgui_demo.cpp
	${imgui_external_SOURCE_DIR}/imgui_draw.cpp
	${imgui_external_SOURCE_DIR}/imgui_tables.cpp
	${imgui_external_SOURCE_DIR}/imgui_widgets.cpp
	${imgui_external_SOURCE_DIR}/backends/imgui_impl_sdl3.h
	${imgui_external_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
	${imgui_external_SOURCE_DIR}/backends/imgui_impl_opengl3.h
	${imgui_external_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ${imgui_external_SOURCE_DIR} ${imgui_external_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC SDL3::SDL3-static)
set_target_properties(imgui PROPERTIES
	CXX_STANDARD  17
)

# Dear Bindings
#FetchContent_Declare(dcimgui_external
#	URL https://github.com/dearimgui/dear_bindings/releases/download/DearBindings_v0.18_ImGui_v1.92.7-docking/DearBindings_v0.18_ImGui_v1.92.7-docking.zip
#	EXCLUDE_FROM_ALL
#)
#FetchContent_MakeAvailable(dcimgui_external)

set(dcimgui_external_SOURCE_DIR ${CMAKE_SOURCE_DIR}/extern/dcimgui)

add_library(dcimgui
	${dcimgui_external_SOURCE_DIR}/dcimgui.h
	${dcimgui_external_SOURCE_DIR}/dcimgui.cpp
	${dcimgui_external_SOURCE_DIR}/dcimgui_internal.h
	${dcimgui_external_SOURCE_DIR}/dcimgui_internal.cpp
	${dcimgui_external_SOURCE_DIR}/backends/dcimgui_impl_sdl3.h
	${dcimgui_external_SOURCE_DIR}/backends/dcimgui_impl_sdl3.cpp
	${dcimgui_external_SOURCE_DIR}/backends/dcimgui_impl_opengl3.h
	${dcimgui_external_SOURCE_DIR}/backends/dcimgui_impl_opengl3.cpp
)
target_include_directories(dcimgui PUBLIC ${dcimgui_external_SOURCE_DIR} ${dcimgui_external_SOURCE_DIR}/backends)
target_link_libraries(dcimgui PUBLIC imgui SDL3::SDL3-static)
set_target_properties(dcimgui PROPERTIES
	CXX_STANDARD  17
)

if(MSVC)
	add_compile_options(/utf-8 /MP /W4)
else()
	add_compile_options(-Wall -Wextra -pedantic)
endif()

set(EXAMPLE_FILES
	src/main.c
	src/imgui_utils.h
	src/imgui_utils.c
	src/utils.h
	src/utils.c
	src/edit_shape.h
	src/edit_shape.c
	src/edit_tags.h
	src/edit_tags.c
	src/edit_gradient.h
	src/edit_gradient.c
	src/edit_todos.h
	src/edit_todos.c
	src/edit_actions.h
	src/edit_actions.c
	src/edit_nodes.h
	src/edit_nodes.c
)

add_executable(example ${EXAMPLE_FILES})

target_include_directories(example PRIVATE src ${GLAD_DIR}/include)
target_link_libraries(example PUBLIC SDL3::SDL3-static imgui dcimgui)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${EXAMPLE_FILES})

add_custom_command(
	TARGET example POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
		${CMAKE_SOURCE_DIR}/data/
		${CMAKE_CURRENT_BINARY_DIR}/data/
)

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
