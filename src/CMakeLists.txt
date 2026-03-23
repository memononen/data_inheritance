include(FetchContent)

if(POLICY CMP0135)
	cmake_policy(SET CMP0135 NEW) # avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24+
endif()


# glad for OpenGL API
set(GLAD_DIR ${CMAKE_SOURCE_DIR}/extern/glad)

# SDL
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

# IMGUI
FetchContent_Declare(imgui_external
	GIT_REPOSITORY https://github.com/ocornut/imgui.git
	GIT_TAG 2a1b69f05748ad909f03acf4533447cac1331611 # docking 1.92.6
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
if(MSVC)
    target_compile_options(imgui PRIVATE /MP)
endif()


# DCIMGUI
FetchContent_Declare(dcimgui_external
#	URL https://github.com/dearimgui/dear_bindings/actions/runs/22354685115/artifacts/5636220484
	URL file://C:/Users/memon/Downloads/dear_bindings_docking_clang_withgeneratedefaultargfunctions.zip
	EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(dcimgui_external)

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
if(MSVC)
    target_compile_options(dcimgui PRIVATE /MP)
endif()

add_compile_options("$<$<COMPILE_LANG_AND_ID:C,MSVC>:/utf-8>")
add_compile_options("$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/utf-8>")


set(EXAMPLE_FILES
	main.c
	imgui_utils.h
	imgui_utils.c
	utils.h
	utils.c
	edit_shape.h
	edit_shape.c
	edit_tags.h
	edit_tags.c
	edit_gradient.h
	edit_gradient.c
	edit_todos.h
	edit_todos.c
	edit_actions.h
	edit_actions.c
	edit_nodes.h
	edit_nodes.c
)

add_executable(example ${EXAMPLE_FILES})

set_target_properties(example PROPERTIES
	C_STANDARD 17
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
)

if(MSVC)
    target_compile_options(example PRIVATE /MP)
endif()

target_include_directories(example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}  ${GLAD_DIR}/include)
target_link_libraries(example PUBLIC SDL3::SDL3-static imgui dcimgui)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${EXAMPLE_FILES})

add_custom_command(
	TARGET example POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
			${CMAKE_SOURCE_DIR}/data/
			${CMAKE_CURRENT_BINARY_DIR}/data/)
