// SPDX-FileCopyrightText: 2025 Mikko Mononen
// SPDX-License-Identifier: MIT

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <glad/gl.h>

#define SDL_MAIN_USE_CALLBACKS
#include <float.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_init.h>

#include "dcimgui.h"
#include "dcimgui_internal.h"
#include "imgui_utils.h"
#include "backends/dcimgui_impl_sdl3.h"
#include "backends/dcimgui_impl_opengl3.h"

#include "edit_shape.h"
#include "edit_tags.h"
#include "edit_gradient.h"
#include "edit_actions.h"
#include "edit_nodes.h"
#include "edit_todos.h"
#include "utils.h"


bool g_done = false;
SDL_Window* g_window = NULL;
SDL_GLContext g_gl_context = NULL;

int32_t g_ticks_left = 1;
uint32_t g_tick_event = 0;
bool g_redraw_handled = false;

void request_tick(int32_t tick_count)
{
	if (tick_count > g_ticks_left)
		g_ticks_left = tick_count;

	SDL_Event event = {.type = g_tick_event};
	SDL_PushEvent(&event);
}

// Periodic timer that will redraw in case no activity.
Uint32 handle_redraw_timer(void* userdata, SDL_TimerID timerID, Uint32 interval)
{
	(void)userdata;
	(void)timerID;
	if (g_redraw_handled)
		request_tick(0);
	return interval;
}


void data_inheritance_demo(void)
{
	static node_ref_array_t base_nodes = {0};
	static node_ref_array_t base_nodes_saved = {0};
	static node_ref_array_t derived_nodes = {0};

	static gradient_t base_grad = {0};
	static gradient_t base_grad_saved = {0};
	static gradient_t derived_grad = {0};

	static tag_container_t base_tags = {0};
	static tag_container_t base_tags_saved = {0};
	static tag_container_t derived_tags = {0};

	static collision_shape_t base_shape = {0};
	static collision_shape_t base_shape_saved = {0};
	static collision_shape_t derived_shape = {0};

	static action_map_t base_actions = {0};
	static action_map_t base_actions_saved = {0};
	static action_map_t derived_actions = {0};

	static todo_list_t base_todos = {0};
	static todo_list_t base_todos_saved = {0};
	static todo_list_t derived_todos = {0};

	static bool initialized = false;
	if (!initialized) {
		// Shape
		base_shape.shape = SHAPE_CIRCLE;
		base_shape.size = 24.f;
		base_shape.hit_flags = HIT_STATIC | HIT_SENSOR;
		base_shape_saved = base_shape;
		shape_update_inherited_data(&base_shape_saved, &derived_shape);

		// Tags
		tags_add(&base_tags, 11);
		tags_add(&base_tags, 21);
		tags_add(&base_tags, 31);
		base_tags_saved = base_tags;
		tags_update_inherited_data(&base_tags_saved, &derived_tags);

		// Grad
		gradient_add(&base_grad, 0.f, (ImVec4){0.5f,.75f,1.f,1.f});
		gradient_add(&base_grad, 1.f, (ImVec4){1.f,.75f,0.125f,1.f});
		base_grad_saved = base_grad;
		gradient_update_inherited_data(&base_grad_saved, &derived_grad);

		// TODOs
		todos_add(&base_todos, "Buy Milk");
		todos_add(&base_todos, "Milk the Cows");
		todos_add(&base_todos, "Shave the Yaks");
		base_todos_saved = base_todos;
		todos_update_inherited_data(&base_todos_saved, &derived_todos);

		// Actions
		actions_add(&base_actions, BUTTON_A, ACTION_FIRE);
		actions_add(&base_actions, BUTTON_B, ACTION_MELEE);
		actions_add(&base_actions, BUTTON_Y, ACTION_DASH);
		base_actions_saved = base_actions;
		actions_update_inherited_data(&base_actions_saved, &derived_actions);

		// Nodes
		nodes_add(&base_nodes, gen_name("Item"));
		nodes_add(&base_nodes, gen_name("Item"));
		nodes_add(&base_nodes, gen_name("Item"));
		nodes_add(&base_nodes, gen_name("Item"));
		base_nodes_saved = base_nodes;
		nodes_update_inherited_data(&base_nodes_saved, &derived_nodes);

		initialized = true;
	}


	static bool open = true;
	if (ImGui_Begin("Data Inheritance", &open, 0)) {
		ImGui_SetWindowSize((ImVec2){1200,1000}, ImGuiCond_Once );

		// Shape
		if (ImGui_BeginTable("examples", 3, 0)) {
			ImGui_TableSetupColumnEx("", ImGuiTableColumnFlags_WidthStretch, 1.0f, 0);
			ImGui_TableSetupColumnEx("", ImGuiTableColumnFlags_WidthFixed, 30, 0);
			ImGui_TableSetupColumnEx("", ImGuiTableColumnFlags_WidthStretch, 1.0f, 0);

			ImGui_TableNextColumn();
			ImGui_PushID("shape_base");

			ImGui_BeginPack(ImGui_GetRowRect(ImGui_DefaultRowHeight));

			ImGui_PackNextSlot(ImGui_MeasureIconButton(), ImGuiPack_End, ImGuiAlign_Center);
			if (ImGui_IconButtonEx(ICON_CHECK, base_shape.has_changes)) {
				base_shape_saved = base_shape;
				shape_update_inherited_data(&base_shape_saved, &derived_shape);
				base_shape.has_changes = false;
			}
			ImGui_SetItemTooltip("Save changes, and update from Base to Derived.");

			ImGui_PackNextSlotPct(1.f, ImGui_GetFrameHeight(), ImGuiPack_Start, ImGuiAlign_Center);
			ImGui_AlignTextToFramePadding();
			ImGui_TextColoredUnformatted(ImGui_GetColorVec4U32(IM_COL32(255,255,255,128)), "Base");

			ImGui_EndPack();

			edit_collosion_shape(&base_shape, false);
			ImGui_PopID();

			ImGui_TableNextColumn();

			ImGui_TableNextColumn();
			ImGui_PushID("shape_derived");

			ImGui_BeginPack(ImGui_GetRowRect(ImGui_DefaultRowHeight));
			ImGui_PackNextSlotPct(1.f, ImGui_GetFrameHeight(), ImGuiPack_Start, ImGuiAlign_Center);
			ImGui_AlignTextToFramePadding();
			ImGui_TextColoredUnformatted(ImGui_GetColorVec4U32(IM_COL32(255,255,255,128)),"Derived");
			ImGui_EndPack();

			if (edit_collosion_shape(&derived_shape, true)) {
				shape_update_inherited_data(&base_shape_saved, &derived_shape);
			}
			ImGui_PopID();

			ImGui_EndTable();
		}

		ImGui_Separator();

		// Tags
		if (ImGui_BeginTable("examples", 3, 0)) {
			ImGui_TableSetupColumnEx("", ImGuiTableColumnFlags_WidthStretch, 1.0f, 0);
			ImGui_TableSetupColumnEx("", ImGuiTableColumnFlags_WidthFixed, 30, 0);
			ImGui_TableSetupColumnEx("", ImGuiTableColumnFlags_WidthStretch, 1.0f, 0);

			ImGui_TableNextColumn();
			ImGui_PushID("tags_base");

			ImGui_BeginPack(ImGui_GetRowRect(ImGui_DefaultRowHeight));

			ImGui_PackNextSlot(ImGui_MeasureIconButton(), ImGuiPack_End, ImGuiAlign_Center);
			if (ImGui_IconButtonEx(ICON_CHECK, base_tags.has_changes)) {
				base_tags_saved = base_tags;
				tags_update_inherited_data(&base_tags_saved, &derived_tags);
				base_tags.has_changes = false;
			}
			ImGui_SetItemTooltip("Save changes, and update from Base to Derived.");

			ImGui_PackNextSlotPct(1.f, ImGui_GetFrameHeight(), ImGuiPack_Start, ImGuiAlign_Center);
			ImGui_AlignTextToFramePadding();
			ImGui_TextColoredUnformatted(ImGui_GetColorVec4U32(IM_COL32(255,255,255,128)), "Base");

			ImGui_EndPack();

			edit_tags(&base_tags, false);
			ImGui_PopID();

			ImGui_TableNextColumn();

			ImGui_TableNextColumn();
			ImGui_PushID("tags_derived");

			ImGui_BeginPack(ImGui_GetRowRect(ImGui_DefaultRowHeight));
			ImGui_PackNextSlotPct(1.f, ImGui_GetFrameHeight(), ImGuiPack_Start, ImGuiAlign_Center);
			ImGui_AlignTextToFramePadding();
			ImGui_TextColoredUnformatted(ImGui_GetColorVec4U32(IM_COL32(255,255,255,128)),"Derived");
			ImGui_EndPack();

			if (edit_tags(&derived_tags, true)) {
				tags_update_inherited_data(&base_tags_saved, &derived_tags);
			}
			ImGui_PopID();

			ImGui_EndTable();
		}

		ImGui_Separator();

		// Gradient
		if (ImGui_BeginTable("examples", 3, 0)) {
			ImGui_TableSetupColumnEx("", ImGuiTableColumnFlags_WidthStretch, 1.0f, 0);
			ImGui_TableSetupColumnEx("", ImGuiTableColumnFlags_WidthFixed, 30, 0);
			ImGui_TableSetupColumnEx("", ImGuiTableColumnFlags_WidthStretch, 1.0f, 0);

			ImGui_TableNextColumn();
			ImGui_PushID("grad_base");

			ImGui_BeginPack(ImGui_GetRowRect(ImGui_DefaultRowHeight));

			ImGui_PackNextSlot(ImGui_MeasureIconButton(), ImGuiPack_End, ImGuiAlign_Center);
			if (ImGui_IconButtonEx(ICON_CHECK, base_grad.has_changes)) {
				base_grad_saved = base_grad;
				gradient_update_inherited_data(&base_grad_saved, &derived_grad);
				base_grad.has_changes = false;
			}
			ImGui_SetItemTooltip("Save changes, and update from Base to Derived.");

			ImGui_PackNextSlotPct(1.f, ImGui_GetFrameHeight(), ImGuiPack_Start, ImGuiAlign_Center);
			ImGui_AlignTextToFramePadding();
			ImGui_TextColoredUnformatted(ImGui_GetColorVec4U32(IM_COL32(255,255,255,128)),"Base");

			ImGui_EndPack();

			edit_gradient(&base_grad, NULL);
			ImGui_PopID();

			ImGui_TableNextColumn();

			ImGui_TableNextColumn();
			ImGui_PushID("grad_derived");

			ImGui_BeginPack(ImGui_GetRowRect(ImGui_DefaultRowHeight));
			ImGui_PackNextSlotPct(1.f, ImGui_GetFrameHeight(), ImGuiPack_Start, ImGuiAlign_Center);
			ImGui_AlignTextToFramePadding();
			ImGui_TextColoredUnformatted(ImGui_GetColorVec4U32(IM_COL32(255,255,255,128)),"Derived");
			ImGui_EndPack();

			if (edit_gradient(&derived_grad, &base_grad_saved)) {
				gradient_update_inherited_data(&base_grad_saved, &derived_grad);
			}
			ImGui_PopID();

			ImGui_EndTable();
		}

		ImGui_Separator();

		// Todos
		if (ImGui_BeginTable("examples", 3, 0)) {
			ImGui_TableSetupColumnEx("", ImGuiTableColumnFlags_WidthStretch, 1.0f, 0);
			ImGui_TableSetupColumnEx("", ImGuiTableColumnFlags_WidthFixed, 30, 0);
			ImGui_TableSetupColumnEx("", ImGuiTableColumnFlags_WidthStretch, 1.0f, 0);

			ImGui_TableNextColumn();
			ImGui_PushID("todos_base");

			ImGui_BeginPack(ImGui_GetRowRect(ImGui_DefaultRowHeight));

			ImGui_PackNextSlot(ImGui_MeasureIconButton(), ImGuiPack_End, ImGuiAlign_Center);
			if (ImGui_IconButtonEx(ICON_CHECK, base_todos.has_changes)) {
				base_todos_saved = base_todos;
				todos_update_inherited_data(&base_todos_saved, &derived_todos);
				base_todos.has_changes = false;
			}
			ImGui_SetItemTooltip("Save changes, and update from Base to Derived.");

			ImGui_PackNextSlotPct(1.f, ImGui_GetFrameHeight(), ImGuiPack_Start, ImGuiAlign_Center);
			ImGui_AlignTextToFramePadding();
			ImGui_TextColoredUnformatted(ImGui_GetColorVec4U32(IM_COL32(255,255,255,128)), "Base");

			ImGui_EndPack();

			edit_todos(&base_todos, 0);
			ImGui_PopID();

			ImGui_TableNextColumn();

			ImGui_TableNextColumn();
			ImGui_PushID("todos_derived");

			ImGui_BeginPack(ImGui_GetRowRect(ImGui_DefaultRowHeight));
			ImGui_PackNextSlotPct(1.f, ImGui_GetFrameHeight(), ImGuiPack_Start, ImGuiAlign_Center);
			ImGui_AlignTextToFramePadding();
			ImGui_TextColoredUnformatted(ImGui_GetColorVec4U32(IM_COL32(255,255,255,128)),"Derived");
			ImGui_EndPack();

			if (edit_todos(&derived_todos, &base_todos_saved)) {
				todos_update_inherited_data(&base_todos_saved, &derived_todos);
			}
			ImGui_PopID();

			ImGui_EndTable();
		}

		ImGui_Separator();

		// Actions
		if (ImGui_BeginTable("examples", 3, 0)) {
			ImGui_TableSetupColumnEx("", ImGuiTableColumnFlags_WidthStretch, 1.0f, 0);
			ImGui_TableSetupColumnEx("", ImGuiTableColumnFlags_WidthFixed, 30, 0);
			ImGui_TableSetupColumnEx("", ImGuiTableColumnFlags_WidthStretch, 1.0f, 0);

			ImGui_TableNextColumn();
			ImGui_PushID("actions_base");

			ImGui_BeginPack(ImGui_GetRowRect(ImGui_DefaultRowHeight));

			ImGui_PackNextSlot(ImGui_MeasureIconButton(), ImGuiPack_End, ImGuiAlign_Center);
			if (ImGui_IconButtonEx(ICON_CHECK, base_actions.has_changes)) {
				base_actions_saved = base_actions;
				actions_update_inherited_data(&base_actions_saved, &derived_actions);
				base_actions.has_changes = false;
			}
			ImGui_SetItemTooltip("Save changes, and update from Base to Derived.");

			ImGui_PackNextSlotPct(1.f, ImGui_GetFrameHeight(), ImGuiPack_Start, ImGuiAlign_Center);
			ImGui_AlignTextToFramePadding();
			ImGui_TextColoredUnformatted(ImGui_GetColorVec4U32(IM_COL32(255,255,255,128)), "Base");

			ImGui_EndPack();

			edit_actions(&base_actions, 0);
			ImGui_PopID();

			ImGui_TableNextColumn();

			ImGui_TableNextColumn();
			ImGui_PushID("actions_derived");

			ImGui_BeginPack(ImGui_GetRowRect(ImGui_DefaultRowHeight));
			ImGui_PackNextSlotPct(1.f, ImGui_GetFrameHeight(), ImGuiPack_Start, ImGuiAlign_Center);
			ImGui_AlignTextToFramePadding();
			ImGui_TextColoredUnformatted(ImGui_GetColorVec4U32(IM_COL32(255,255,255,128)),"Derived");
			ImGui_EndPack();

			if (edit_actions(&derived_actions, &base_actions_saved)) {
				actions_update_inherited_data(&base_actions_saved, &derived_actions);
			}
			ImGui_PopID();

			ImGui_EndTable();
		}

		ImGui_Separator();

		// Nodes
		if (ImGui_BeginTable("examples", 3, 0)) {
			ImGui_TableSetupColumnEx("", ImGuiTableColumnFlags_WidthStretch, 1.0f, 0);
			ImGui_TableSetupColumnEx("", ImGuiTableColumnFlags_WidthFixed, 30, 0);
			ImGui_TableSetupColumnEx("", ImGuiTableColumnFlags_WidthStretch, 1.0f, 0);

			// Base array
			ImGui_TableNextColumn();
			ImGui_PushID("nodes_base");

			ImGui_BeginPack(ImGui_GetRowRect(ImGui_DefaultRowHeight));

			ImGui_PackNextSlot(ImGui_MeasureIconButton(), ImGuiPack_End, ImGuiAlign_Center);
			if (ImGui_IconButtonEx(ICON_CHECK, base_nodes.has_changes)) {
				base_nodes_saved = base_nodes;
				nodes_update_inherited_data(&base_nodes_saved, &derived_nodes);
				base_nodes.has_changes = false;
			}
			ImGui_SetItemTooltip("Save changes, and update from Base to Derived.");

			ImGui_PackNextSlotPct(1.f, ImGui_GetFrameHeight(), ImGuiPack_Start, ImGuiAlign_Center);
			ImGui_AlignTextToFramePadding();
			ImGui_TextColoredUnformatted(ImGui_GetColorVec4U32(IM_COL32(255,255,255,128)), "Base");

			ImGui_EndPack();

			edit_nodes(&base_nodes, NULL);
			ImGui_PopID();

			ImGui_TableNextColumn();

			ImGui_TableNextColumn();
			ImGui_PushID("nodes_derived");

			ImGui_BeginPack(ImGui_GetRowRect(ImGui_DefaultRowHeight));
			ImGui_PackNextSlotPct(1.f, ImGui_GetFrameHeight(), ImGuiPack_Start, ImGuiAlign_Center);
			ImGui_AlignTextToFramePadding();
			ImGui_TextColoredUnformatted(ImGui_GetColorVec4U32(IM_COL32(255,255,255,128)),"Derived");
			ImGui_EndPack();

			if (edit_nodes(&derived_nodes, &base_nodes_saved)) {
				nodes_update_inherited_data(&base_nodes_saved, &derived_nodes);
			}

			ImGui_PopID();

			ImGui_EndTable();
		}

		ImGui_Separator();


	}
	ImGui_End();
}


SDL_AppResult SDL_Fail()
{
	SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError());
	return SDL_APP_FAILURE;
}


SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
	(void)appstate;
	(void)argc;
	(void)argv;
	// init the library, here we make a window so we only need the Video capabilities.
	if (!SDL_Init(SDL_INIT_VIDEO))
		return SDL_Fail();

	int num_displays;
	SDL_DisplayID* displays = SDL_GetDisplays(&num_displays);
	if (!num_displays)
		return SDL_Fail();
	const SDL_DisplayMode* display_mode = SDL_GetDesktopDisplayMode(displays[0]);
	if (!display_mode)
		return SDL_Fail();

	SDL_free(displays);

	// Redraw only on events, or after 250ms of inactivity.
	SDL_SetHint(SDL_HINT_MAIN_CALLBACK_RATE, "waitevent");
	g_tick_event = SDL_RegisterEvents(1);
	SDL_AddTimer(250, handle_redraw_timer, NULL);

	// GL 3.0 + GLSL 130
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	// create a window
	g_window = SDL_CreateWindow("Array Merge Prototype", display_mode->w - 200, display_mode->h - 200, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
	if (!g_window)
		return SDL_Fail();

	g_gl_context = SDL_GL_CreateContext(g_window);
	if (!g_gl_context)
		return SDL_Fail();

	SDL_GL_MakeCurrent(g_window, g_gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync
	SDL_SetWindowPosition(g_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_ShowWindow(g_window);

	const char* base_path_ptr = SDL_GetBasePath();
	if (!base_path_ptr)
		return SDL_Fail();

	SDL_Log("Application started successfully!");

	int version = gladLoadGL(SDL_GL_GetProcAddress);
	if (version == 0) {
		SDL_Log("Failed to initialize OpenGL context\n");
		return SDL_Fail();
	}


	ImGui_CreateContext(NULL);
	ImGuiIO* io = ImGui_GetIO();
	io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
	io->ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
	io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows

	// Setup Dear ImGui style
	ImGui_StyleColorsDark(NULL);

	ImGuiStyle* style = ImGui_GetStyle();
	setup_imgui_style(style, 1.5f);

	ImFontAtlas_AddFontFromFileTTF(io->Fonts, "data/AtkinsonHyperlegibleNext-Regular.ttf", 13.f, NULL, NULL);

	ImFontConfig font_config = ImFontConfig_Default();
	font_config.MergeMode = true;
	ImFontAtlas_AddFontFromFileTTF(io->Fonts, "data/tabler-icons-subset.ttf", 16.f, &font_config, NULL);


	// Setup Platform/Renderer backends
	cImGui_ImplSDL3_InitForOpenGL(g_window, g_gl_context);
	cImGui_ImplOpenGL3_Init();

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
	(void)appstate;
	// Skip our custom tick event.
	if (event->type == g_tick_event) {
		return SDL_APP_CONTINUE;
	}
	// Draw the screen this many times after each event to allow the imgui stuff to settle.
	g_ticks_left = 3;

	if (event->type == SDL_EVENT_QUIT) {
		g_done = true;
	}

	cImGui_ImplSDL3_ProcessEvent(event);

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
	(void)appstate;

	int32_t win_width, win_height;
	int32_t fb_width, fb_height;

	SDL_GetWindowSize(g_window, &win_width, &win_height);
	SDL_GetWindowSizeInPixels(g_window, &fb_width, &fb_height);

	cImGui_ImplOpenGL3_NewFrame();
	cImGui_ImplSDL3_NewFrame();
	ImGui_NewFrame();

	ImGuiIO* io = ImGui_GetIO();

	// Update and render
	glViewport(0, 0, fb_width, fb_height);

	glClearColor(0.9f, 0.91f, 0.92f, 1.0f);
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

	data_inheritance_demo();

//	static bool show_metrics = true;
//	ImGui_ShowMetricsWindow(&show_metrics);

	ImGui_Render();
	cImGui_ImplOpenGL3_RenderDrawData(ImGui_GetDrawData());

	// Update and Render additional Platform Windows
	if (io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
		SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
		ImGui_UpdatePlatformWindows();
		ImGui_RenderPlatformWindowsDefault();
		SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
	}

	glEnable(GL_DEPTH_TEST);

	SDL_GL_SwapWindow(g_window);

	// Request redraw if we should redraw.
	if (g_ticks_left > 0) {
		request_tick(0);
		g_ticks_left--;
	}
	g_redraw_handled = true;

	return g_done ? SDL_APP_SUCCESS : SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
	(void)appstate;
	(void)result;
	cImGui_ImplOpenGL3_Shutdown();
	cImGui_ImplSDL3_Shutdown();
	ImGui_DestroyContext(NULL);

	if (g_gl_context)
		SDL_GL_DestroyContext(g_gl_context);

	if (g_window)
		SDL_DestroyWindow(g_window);

	SDL_Log("Application quit successfully!");
	SDL_Quit();
}


// Single header lib implementations
#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
