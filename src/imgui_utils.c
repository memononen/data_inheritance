#include "imgui_utils.h"

#include <string.h>

#include "dcimgui_internal.h"

static float g_override_marker_width = 2.f;
static float g_override_marker_space = 1.f;

float get_override_marker_width(void)
{
	return g_override_marker_width;
}

float get_override_marker_space(void)
{
	return g_override_marker_space;
}

void setup_imgui_style(ImGuiStyle* style, float scale)
{
	style->WindowPadding.x = 8.f;
	style->WindowPadding.y = 8.f;

	style->FramePadding.x = 6.f;
	style->FramePadding.y = 4.f;

	style->ItemSpacing.x = 8.f;
	style->ItemSpacing.y = 4.f;

	style->WindowRounding = 2.f;
	style->PopupRounding = 2.f;
	style->FrameRounding = 2.f;
	style->GrabRounding = 2.f;

	style->CellPadding.x = 6.f;
	style->CellPadding.x = 4.f;

	style->TabBarBorderSize = 1.f;
	style->TabRounding = 2.f;
	style->TabBarOverlineSize = 2.f;
	style->TabBorderSize = 1.f;

	ImVec4* colors = style->Colors;

	colors[ImGuiCol_Text]                   = (ImVec4){1.00f, 1.00f, 1.00f, 0.86f};
	colors[ImGuiCol_TextDisabled]           = (ImVec4){1.00f, 1.00f, 1.00f, 0.50f};
	colors[ImGuiCol_WindowBg]               = (ImVec4){0.20f, 0.20f, 0.20f, 1.00f};
	colors[ImGuiCol_ChildBg]                = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_PopupBg]                = (ImVec4){0.13f, 0.13f, 0.13f, 1.00f};
	colors[ImGuiCol_Border]                 = (ImVec4){1.00f, 1.00f, 1.00f, 0.25f};
	colors[ImGuiCol_BorderShadow]           = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_FrameBg]                = (ImVec4){0.00f, 0.00f, 0.00f, 0.50f};
	colors[ImGuiCol_FrameBgHovered]         = (ImVec4){0.00f, 0.00f, 0.00f, 0.25f};
	colors[ImGuiCol_FrameBgActive]          = (ImVec4){0.26f, 0.59f, 0.98f, 0.67f};
	colors[ImGuiCol_TitleBg]                = (ImVec4){0.04f, 0.04f, 0.04f, 1.00f};
	colors[ImGuiCol_TitleBgActive]          = (ImVec4){0.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_TitleBgCollapsed]       = (ImVec4){0.00f, 0.00f, 0.00f, 0.51f};
	colors[ImGuiCol_MenuBarBg]              = (ImVec4){0.14f, 0.14f, 0.14f, 1.00f};
	colors[ImGuiCol_ScrollbarBg]            = (ImVec4){0.02f, 0.02f, 0.02f, 0.53f};
	colors[ImGuiCol_ScrollbarGrab]          = (ImVec4){0.31f, 0.31f, 0.31f, 1.00f};
	colors[ImGuiCol_ScrollbarGrabHovered]   = (ImVec4){0.41f, 0.41f, 0.41f, 1.00f};
	colors[ImGuiCol_ScrollbarGrabActive]    = (ImVec4){0.51f, 0.51f, 0.51f, 1.00f};
	colors[ImGuiCol_CheckMark]              = (ImVec4){0.26f, 0.59f, 0.98f, 1.00f};
	colors[ImGuiCol_SliderGrab]             = (ImVec4){0.24f, 0.52f, 0.88f, 1.00f};
	colors[ImGuiCol_SliderGrabActive]       = (ImVec4){0.26f, 0.59f, 0.98f, 1.00f};
	colors[ImGuiCol_Button]                 = (ImVec4){1.00f, 1.00f, 1.00f, 0.14f};
	colors[ImGuiCol_ButtonHovered]          = (ImVec4){0.26f, 0.59f, 0.98f, 1.00f};
	colors[ImGuiCol_ButtonActive]           = (ImVec4){0.06f, 0.53f, 0.98f, 1.00f};
	colors[ImGuiCol_Header]                 = (ImVec4){1.00f, 1.00f, 1.00f, 0.13f};
	colors[ImGuiCol_HeaderHovered]          = (ImVec4){0.26f, 0.59f, 0.98f, 0.80f};
	colors[ImGuiCol_HeaderActive]           = (ImVec4){0.26f, 0.59f, 0.98f, 1.00f};
	colors[ImGuiCol_Separator]              = (ImVec4){1.00f, 1.00f, 1.00f, 0.25f};
	colors[ImGuiCol_SeparatorHovered]       = (ImVec4){0.26f, 0.59f, 0.98f, 0.67f};
	colors[ImGuiCol_SeparatorActive]        = (ImVec4){0.26f, 0.59f, 0.98f, 0.95f};
	colors[ImGuiCol_ResizeGrip]             = (ImVec4){0.26f, 0.59f, 0.98f, 0.20f};
	colors[ImGuiCol_ResizeGripHovered]      = (ImVec4){0.26f, 0.59f, 0.98f, 0.67f};
	colors[ImGuiCol_ResizeGripActive]       = (ImVec4){0.26f, 0.59f, 0.98f, 0.95f};
	colors[ImGuiCol_InputTextCursor]        = (ImVec4){1.00f, 1.00f, 1.00f, 1.00f};
	colors[ImGuiCol_TabHovered]             = (ImVec4){0.26f, 0.59f, 0.98f, 0.80f};
	colors[ImGuiCol_Tab]                    = (ImVec4){1.00f, 1.00f, 1.00f, 0.00f};
	colors[ImGuiCol_TabSelected]            = (ImVec4){0.22f, 0.22f, 0.22f, 1.00f};
	colors[ImGuiCol_TabSelectedOverline]    = (ImVec4){0.26f, 0.59f, 0.98f, 1.00f};
	colors[ImGuiCol_TabDimmed]              = (ImVec4){1.00f, 1.00f, 1.00f, 0.00f};
	colors[ImGuiCol_TabDimmedSelected]      = (ImVec4){0.22f, 0.22f, 0.22f, 1.00f};
	colors[ImGuiCol_TabDimmedSelectedOverline]  = (ImVec4){0.50f, 0.50f, 0.50f, 0.00f};
	colors[ImGuiCol_DockingPreview]         = (ImVec4){0.26f, 0.59f, 0.98f, 0.70f};
	colors[ImGuiCol_DockingEmptyBg]         = (ImVec4){0.08f, 0.08f, 0.08f, 1.00f};
	colors[ImGuiCol_PlotLines]              = (ImVec4){0.61f, 0.61f, 0.61f, 1.00f};
	colors[ImGuiCol_PlotLinesHovered]       = (ImVec4){1.00f, 0.43f, 0.35f, 1.00f};
	colors[ImGuiCol_PlotHistogram]          = (ImVec4){0.90f, 0.70f, 0.00f, 1.00f};
	colors[ImGuiCol_PlotHistogramHovered]   = (ImVec4){1.00f, 0.60f, 0.00f, 1.00f};
	colors[ImGuiCol_TableHeaderBg]          = (ImVec4){1.00f, 1.00f, 1.00f, 0.09f};
	colors[ImGuiCol_TableBorderStrong]      = (ImVec4){1.00f, 1.00f, 1.00f, 0.25f};
	colors[ImGuiCol_TableBorderLight]       = (ImVec4){1.00f, 1.00f, 1.00f, 0.13f};
	colors[ImGuiCol_TableRowBg]             = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_TableRowBgAlt]          = (ImVec4){1.00f, 1.00f, 1.00f, 0.05f};
	colors[ImGuiCol_TextLink]               = (ImVec4){0.26f, 0.59f, 0.98f, 1.00f};
	colors[ImGuiCol_TextSelectedBg]         = (ImVec4){0.26f, 0.59f, 0.98f, 0.35f};
	colors[ImGuiCol_TreeLines]              = (ImVec4){1.00f, 1.00f, 1.00f, 0.38f};
	colors[ImGuiCol_DragDropTarget]         = (ImVec4){1.00f, 1.00f, 0.00f, 0.90f};
	colors[ImGuiCol_DragDropTargetBg]       = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_UnsavedMarker]          = (ImVec4){1.00f, 1.00f, 1.00f, 1.00f};
	colors[ImGuiCol_NavCursor]              = (ImVec4){0.26f, 0.59f, 0.98f, 1.00f};
	colors[ImGuiCol_NavWindowingHighlight]  = (ImVec4){1.00f, 1.00f, 1.00f, 0.70f};
	colors[ImGuiCol_NavWindowingDimBg]      = (ImVec4){0.80f, 0.80f, 0.80f, 0.20f};
	colors[ImGuiCol_ModalWindowDimBg]       = (ImVec4){0.80f, 0.80f, 0.80f, 0.35f};

	style->FontScaleDpi = scale;
	ImGuiStyle_ScaleAllSizes(style, scale);

	g_override_marker_width = ImMaxf(2.f,IM_TRUNC(style->ItemSpacing.x / 4.f));
	g_override_marker_space = ImMaxf(1.f, IM_TRUNC(g_override_marker_width/2.f));

}

//
// Fantasy ImGui API
//

ImVec2 ImGui_MeasureIconButton(void)
{
	float button_size = ImGui_GetFrameHeight();
	return (ImVec2){button_size,button_size};
}

bool ImGui_IconButton(const char* icon)
{
	float button_size = ImGui_GetFrameHeight();

	ImGui_PushStyleColor(ImGuiCol_Button, IM_COL32_BLACK_TRANS);
	ImGui_PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255,255,255,32));
	ImGui_PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(255,255,255,64));

	ImGui_PushStyleVarImVec2(ImGuiStyleVar_FramePadding, (ImVec2){0,0});

	bool res = ImGui_ButtonEx(icon, (ImVec2){button_size,button_size});

	ImGui_PopStyleVarEx(1);
	ImGui_PopStyleColorEx(3);

	return res;
}

bool ImGui_IconButtonEx(const char* icon, bool is_enabled)
{
	ImGui_PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.35f);
	ImGui_BeginDisabled(!is_enabled);
	bool res = ImGui_IconButton(icon);
	ImGui_EndDisabled();
	ImGui_PopStyleVarEx(1);
	return res;
}

bool ImGui_IconButtonColored(const char* icon, ImU32 color)
{
	ImGui_PushStyleColor(ImGuiCol_Text, color);
	bool res = ImGui_IconButton(icon);
	ImGui_PopStyleColor();
	return res;
}

bool ImGui_IconButtonColoredEx(const char* icon, ImU32 color, bool is_enabled)
{
	ImGui_PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.35f);
	ImGui_BeginDisabled(!is_enabled);
	ImGui_PushStyleColor(ImGuiCol_Text, color);
	bool res = ImGui_IconButton(icon);
	ImGui_PopStyleColor();
	ImGui_EndDisabled();
	ImGui_PopStyleVarEx(1);
	return res;
}

ImVec2 ImGui_MeasureIcon(void)
{
	float button_size = ImGui_GetFrameHeight();
	return (ImVec2){button_size,button_size};
}

void ImGui_Icon(const char* icon)
{
	const float button_size = ImGui_GetFrameHeight();
	ImGui_AlignTextToFramePadding();
	ImGui_TextAligned(0.5f, button_size, icon);
}

void ImGui_IconColored(const char* icon, ImU32 color)
{
	ImGui_PushStyleColor(ImGuiCol_Text, color);
	ImGui_Icon(icon);
	ImGui_PopStyleColor();
}

void ImGui_TextAlignedColored(float align_x, float size_x, ImU32 color, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	ImGui_PushStyleColor(ImGuiCol_Text, color);
	ImGui_AlignTextToFramePadding();
	ImGui_TextAlignedV(align_x, size_x, fmt, args);
	ImGui_PopStyleColor();
	va_end(args);
}

void ImGui_SamelineWrapped(float width)
{
	const ImGuiStyle* style = ImGui_GetStyle();
	width += style->ItemSpacing.x + 1;

	ImGui_SameLine();
	ImVec2 avail = ImGui_GetContentRegionAvail();
	if (width > avail.x)
		ImGui_NewLine();
}

ImVec2 ImGui_MeasureFrame(float width_mult)
{
	float frame_h = ImGui_GetFrameHeight();
	return (ImVec2){
		.x = frame_h * width_mult,
		.y = frame_h,
	};
}

ImVec2 ImGui_MeasureTextUnformatted(const char* text)
{
	ImVec2 text_size = ImGui_CalcTextSizeEx(text, NULL, true, -1.f);

	// TODO: this is done in order to count the text baseline offset.
	text_size.y = ImGui_GetFrameHeight();

	return text_size;
}

ImVec2 ImGui_MeasureText(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	const char* buf = NULL;
	const char* buf_end = NULL;
	cImFormatStringToTempBufferV(&buf, &buf_end, fmt, args);
	va_end(args);

	ImVec2 text_size = ImGui_CalcTextSizeEx(buf, buf_end, true, -1.f);

	// TODO: this is done in order to count the text baseline offset.
	text_size.y = ImGui_GetFrameHeight();

	return text_size;
}

ImRect ImGui_GetItemRect(void)
{
	return (ImRect) {
		.Min = ImGui_GetItemRectMin(),
		.Max = ImGui_GetItemRectMax(),
	};
}

ImRect ImGui_GetItemContentRect(void)
{
	ImGuiStyle* style = ImGui_GetStyle();
	ImGui_PushStyleVarImVec2(ImGuiStyleVar_ItemSpacing, (ImVec2){0,0});

	ImRect content_bb = {
		.Min = ImGui_GetItemRectMin(),
		.Max = ImGui_GetItemRectMax(),
	};

	ImGui_SameLine();

	ImVec2 cursor_pos = ImGui_GetCursorScreenPos();
	const float pad_left = ImMaxf(cursor_pos.x - content_bb.Min.x, style->FramePadding.x);
	const float pad_right = style->FramePadding.x;
	content_bb.Min.x += pad_left;
	content_bb.Max.x -= pad_right;

	ImGui_PopStyleVar();

	return content_bb;
}

ImRect ImGui_GetRowRect(float height)
{
	ImVec2 avail = ImGui_GetContentRegionAvail();
	if (height < 0.f)
		height = ImGui_GetFrameHeight();
	ImGui_Dummy((ImVec2){avail.x, height});
	ImRect row_bb = {
		.Min = ImGui_GetItemRectMin(),
		.Max = ImGui_GetItemRectMax(),
	};
	return row_bb;
}

void ImGui_SetNextItemRect(ImVec2 bb_min, ImVec2 bb_max)
{
	ImGui_SetCursorScreenPos(bb_min);
	ImGui_SetNextItemWidth(bb_max.x - bb_min.x);
}

//
// Pack Layout
//

enum {
	ImGuiLayout_MaxStack = 16,
};

typedef struct ImGui_PackLayout {
	ImRect rect;
	ImVec2 initial_cursor_pos;
} ImGui_PackLayout;

typedef struct ImGui_PackLayoutStack {
	ImGui_PackLayout stack[ImGuiLayout_MaxStack];
	int stack_idx;
} ImGui_PackLayoutStack;

ImGui_PackLayoutStack g_pack_layout = { .stack_idx = -1 };

void ImGui_BeginPack(ImRect rect)
{
	assert((g_pack_layout.stack_idx + 1) < ImGuiLayout_MaxStack);
	g_pack_layout.stack_idx++;
	ImGui_PackLayout* layout = &g_pack_layout.stack[g_pack_layout.stack_idx];
	memset(layout, 0, sizeof(ImGui_PackLayout));
	layout->rect = rect;
//	layout->initial_cursor_pos = ImGui_GetCursorScreenPos();
	ImGui_BeginGroup();
}

void ImGui_PackAdvance(ImGuiLayoutPack pack, float spacing)
{
	if (spacing < 0.0f)
		spacing = ImGui_GetStyle()->ItemSpacing.x;

	ImGui_PackLayout* layout = &g_pack_layout.stack[g_pack_layout.stack_idx];

	const float avail_width = layout->rect.Max.x - layout->rect.Min.x;
	spacing = ImMinf(spacing, avail_width);

	// Cut
	if (pack == ImGuiPack_Start) {
		layout->rect.Min.x += spacing;
	} else if (pack == ImGuiPack_End) {
		layout->rect.Max.x -= spacing;
	}
}

ImRect ImGui_PackNextSlotPct(float width_percent, float height, ImGuiLayoutPack pack, ImGuiLayoutAlign align)
{
	return ImGui_PackNextSlotPctEx(width_percent, height, pack, align, ImGui_GetStyle()->ItemSpacing.x);
}

ImRect ImGui_PackNextSlotPctEx(float width_percent, float height, ImGuiLayoutPack pack, ImGuiLayoutAlign align, float spacing_after)
{
	ImGui_PackLayout* layout = &g_pack_layout.stack[g_pack_layout.stack_idx];
	const float row_width = layout->rect.Max.x - layout->rect.Min.x;
	return ImGui_PackNextSlotEx((ImVec2){ IM_TRUNC(row_width * width_percent), height}, pack, align, spacing_after);
}

ImRect ImGui_PackNextSlot(ImVec2 size, ImGuiLayoutPack pack, ImGuiLayoutAlign align)
{
	return ImGui_PackNextSlotEx(size, pack, align, ImGui_GetStyle()->ItemSpacing.x);
}

ImRect ImGui_PackNextSlotEx(ImVec2 size, ImGuiLayoutPack pack, ImGuiLayoutAlign align, float spacing_after)
{
	assert(g_pack_layout.stack_idx >= 0 && g_pack_layout.stack_idx < ImGuiLayout_MaxStack);
	ImGui_PackLayout* layout = &g_pack_layout.stack[g_pack_layout.stack_idx];

	if (spacing_after < 0.0f)
		spacing_after = ImGui_GetStyle()->ItemSpacing.x;

	const float avail_width = layout->rect.Max.x - layout->rect.Min.x;
	spacing_after = ImMinf(spacing_after, avail_width);
	const float req_width = ImMinf(size.x + spacing_after, avail_width);	// requested space clamped to max avail size
	const float width = ImMaxf(0.f, req_width - spacing_after); // requested space sans spacing

	// Pack and cut
	ImRect rect = {0};
	if (pack == ImGuiPack_Start) {
		rect.Min.x = layout->rect.Min.x;
		rect.Min.y = layout->rect.Min.y;
		rect.Max.x = layout->rect.Min.x + width;
		rect.Max.y = layout->rect.Max.y;
		layout->rect.Min.x += req_width;
	} else if (pack == ImGuiPack_End) {
		rect.Min.x = layout->rect.Max.x - width;
		rect.Min.y = layout->rect.Min.y;
		rect.Max.x = layout->rect.Max.x;
		rect.Max.y = layout->rect.Max.y;
		layout->rect.Max.x -= req_width;
	}

	// Align
	if (align == ImGuiAlign_Start) {
		rect.Max.y = rect.Min.y + size.y;
	} else if (align == ImGuiAlign_Center) {
		const float y = rect.Min.y;
		const float h = rect.Max.y - rect.Min.y;
		rect.Min.y = IM_TRUNC(y + h * 0.5f - size.y * 0.5f);
		rect.Max.y = rect.Min.y + size.y;
	} else { // End
		rect.Min.y = rect.Max.y - size.y;
	}

	ImGui_SetNextItemRect(rect.Min, rect.Max);

/*	{
		// Debug layout
		ImDrawList* draw_list = ImGui_GetWindowDrawList();
		ImDrawList_AddRect(draw_list, rect.Min, rect.Max, IM_COL32(255,255,0,128));
	}*/

	return rect;
}

ImRect ImGui_EndPack(void)
{
	assert(g_pack_layout.stack_idx >= 0);
	ImGui_PackLayout* layout = &g_pack_layout.stack[g_pack_layout.stack_idx];
	ImRect remainder = layout->rect;
//	ImGui_SetCursorScreenPos(layout->initial_cursor_pos);
	ImGui_EndGroup();
	g_pack_layout.stack_idx--;

	return remainder;
}


//
// Custom
//

ImVec2 measure_override_marker(void)
{
	float frame_h = ImGui_GetFrameHeight();
	return (ImVec2){
		.x = get_override_marker_width(),
		.y = frame_h,
	};
}

void override_marker(bool is_overridden, bool has_overrides)
{
	float frame_h = ImGui_GetFrameHeight();
	ImGuiStyle* style = ImGui_GetStyle();

	ImGui_Dummy((ImVec2){get_override_marker_width(), frame_h});
	ImVec2 bmin = ImGui_GetItemRectMin();
	ImVec2 bmax = ImGui_GetItemRectMax();

	if (is_overridden || has_overrides) {
		ImDrawList* draw_list = ImGui_GetWindowDrawList();
		ImVec2 marker_bmin = { bmin.x, bmin.y };
		ImVec2 marker_bmax = { bmin.x + get_override_marker_width(), bmax.y };
		ImDrawList_AddRectFilled(draw_list, marker_bmin, marker_bmax, is_overridden ? OVERRIDE_COLOR : OVERRIDE_COLOR_DIM);
	}
}

bool item_override_marker(bool is_override)
{
	bool result = false;

	ImRect rect = ImGui_GetItemRect();

	if (is_override) {
		ImDrawList* draw_list = ImGui_GetWindowDrawList();
		ImDrawList_AddRectFilled(draw_list,
			(ImVec2){rect.Min.x - get_override_marker_width() - get_override_marker_space(), rect.Min.y},
			(ImVec2){rect.Min.x - get_override_marker_space(), rect.Max.y},
			OVERRIDE_COLOR);
	}

	if (ImGui_IsMouseReleased(ImGuiMouseButton_Right) && ImGui_IsMouseHoveringRect(rect.Min, rect.Max))
		ImGui_OpenPopup("##override_popup", 0);

	if (ImGui_BeginPopup("##override_popup", 0)) {
		ImGui_BeginDisabled(!is_override);
		if (ImGui_MenuItemWithIcon("Revert Change", ICON_ARROW_BACK)) {
			result = true;
		}
		ImGui_EndDisabled();
		ImGui_EndPopup();
	}
	return result;
}

bool override_marker_box(ImRect rect, bool is_override)
{
	bool result = false;

	if (is_override) {
		ImDrawList* draw_list = ImGui_GetWindowDrawList();
		ImDrawList_AddRectFilled(draw_list,
			(ImVec2){rect.Min.x - get_override_marker_width() - get_override_marker_space(), rect.Min.y},
			(ImVec2){rect.Min.x - get_override_marker_space(), rect.Max.y},
			OVERRIDE_COLOR);
	}

	if (ImGui_IsMouseReleased(ImGuiMouseButton_Right) && ImGui_IsMouseHoveringRect(rect.Min, rect.Max))
		ImGui_OpenPopup("##override_popup", 0);

	if (ImGui_BeginPopup("##override_popup", 0)) {
		ImGui_BeginDisabled(!is_override);
		if (ImGui_MenuItemWithIcon("Revert Change", ICON_ARROW_BACK)) {
			result = true;
		}
		ImGui_EndDisabled();
		ImGui_EndPopup();
	}
	return result;
}



array_header_state_t edit_array_header(const char* label, int32_t item_count, uint32_t flags)
{
	array_header_state_t res = {0};

	ImDrawList* draw_list = ImGui_GetWindowDrawList();

	ImGui_PushStyleColor(ImGuiCol_Header, ARRAY_HEADER_BG_COLOR);
	ImGui_PushStyleColor(ImGuiCol_HeaderHovered, ARRAY_HEADER_BG_COLOR);
	ImGui_PushStyleColor(ImGuiCol_HeaderActive, ARRAY_HEADER_BG_COLOR_ACTIVE);

	res.is_open = ImGui_CollapsingHeader("##header", ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow);
	res.is_tooltip_hovered = ImGui_IsItemHovered(ImGuiHoveredFlags_ForTooltip);

	ImGui_PopStyleColorEx(3);

	ImRect header_bb = ImGui_GetItemRect();
	if (flags & (ARRAY_HEADER_IS_MODIFIED | ARRAY_HEADER_HAS_MODIFIED_ITEMS)) {
		// Modified marker
		if (flags & (ARRAY_HEADER_IS_MODIFIED | ARRAY_HEADER_HAS_MODIFIED_ITEMS)) {
			ImVec2 marker_bmin = header_bb.Min;
			ImVec2 marker_bmax = { header_bb.Min.x + get_override_marker_width(), header_bb.Max.y };
			ImDrawList_AddRectFilled(draw_list, marker_bmin, marker_bmax, (flags & ARRAY_HEADER_IS_MODIFIED) ? OVERRIDE_COLOR : OVERRIDE_COLOR_DIM);
		}
	}

	ImRect header_content_bb = ImGui_GetItemContentRect();
	ImGui_BeginPack(header_content_bb);

	// Label
	ImVec2 label_size = ImGui_MeasureTextUnformatted(label);
	ImGui_PackNextSlot(label_size, ImGuiPack_Start, ImGuiAlign_Center);
	ImGui_TextUnformatted(label);



	// Revert
	if (flags & ARRAY_HEADER_HAS_REVERT) {
		ImGui_PackNextSlotEx(ImGui_MeasureIconButton(), ImGuiPack_End, ImGuiAlign_Center, 0.f);
		res.revert_is_clicked = ImGui_IconButtonEx(ICON_ARROW_BACK, flags & ARRAY_HEADER_ALLOW_REVERT);
		res.revert_is_tooltip_hovered = ImGui_IsItemHovered(ImGuiHoveredFlags_ForTooltip);
	}

	// Plus button
	if (flags & ARRAY_HEADER_ALLOW_ADD) {
		ImRect add_bb = ImGui_PackNextSlotEx(ImGui_MeasureIconButton(), ImGuiPack_End, ImGuiAlign_Center, 0.f);
		res.add_is_clicked = ImGui_IconButton(ICON_PLUS);
		res.add_is_tooltip_hovered = ImGui_IsItemHovered(ImGuiHoveredFlags_ForTooltip);
		res.menu_pos = (ImVec2){ add_bb.Min.x, add_bb.Max.y };
	}

	// Inherit button
	if (flags & ARRAY_HEADER_ALLOW_INHERIT) {
		ImRect link_bb = ImGui_PackNextSlotEx(ImGui_MeasureIconButton(), ImGuiPack_End, ImGuiAlign_Center, 0.f);
		res.inherit_is_clicked = ImGui_IconButton(ICON_COPY_PLUS);
		res.inherit_is_tooltip_hovered = ImGui_IsItemHovered(ImGuiHoveredFlags_ForTooltip);
		res.menu_pos = (ImVec2){ link_bb.Min.x, link_bb.Max.y };
	}

	ImGui_PackAdvance(ImGuiPack_End, ImGui_DefaultSpacing);

	// Deleted items indicator.
	if (flags & ARRAY_HEADER_HAS_REMOVED_ITEMS) {
		const char* remove_str = ICON_POINT_FILLED;
		ImVec2 marker_size = ImGui_MeasureTextUnformatted(remove_str);
		ImGui_PackNextSlotEx(marker_size, ImGuiPack_End, ImGuiAlign_Center, 0.f);

		ImGui_AlignTextToFramePadding();
		ImGui_TextColored(ImGui_GetColorVec4U32(OVERRIDE_COLOR_REMOVE),ICON_POINT_FILLED);

		res.removed_is_tooltip_hovered = ImGui_IsItemHovered(ImGuiHoveredFlags_ForTooltip);
	}

	// Item Count
	if (item_count >= 0) {
		ImGuiStyle* style = ImGui_GetStyle();
		ImGui_PushFontFloat(NULL, style->FontSizeBase * 0.85f);

		ImVec2 count_size = ImGui_MeasureFrame(3.f);
		ImGui_PackNextSlotEx(count_size, ImGuiPack_End, ImGuiAlign_Center, 0.f);

		ImGui_AlignTextToFramePadding();
		if (item_count == 0)
			ImGui_TextAlignedColored(1.f, count_size.x, IM_COL32(255,255,255,96), "Empty");
		else if (item_count == 1)
			ImGui_TextAlignedColored(1.f, count_size.x, IM_COL32(255,255,255,96), "%d Item", item_count);
		else
			ImGui_TextAlignedColored(1.f, count_size.x, IM_COL32(255,255,255,96), "%d Items", item_count);

		if (flags & ARRAY_HEADER_HAS_REMOVED_ITEMS)
			res.removed_is_tooltip_hovered = ImGui_IsItemHovered(ImGuiHoveredFlags_ForTooltip);

		ImGui_PopFont();
	}


	ImGui_EndPack();

	return res;
}
