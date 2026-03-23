#pragma once

#include <float.h>

#include "dcimgui.h"
#include "dcimgui_internal.h"

// pyftsubset tabler-icons.ttf --unicodes=U+eb55,U+eb0b,U+ea0c,U+f698,U+ea45,U+ea5e,U+ec00,U+ea9a,U+f7ec,U+efa5,U+f0d6,U+fdae,U+f0d8,U+fc15,U+ea94,U+f6f0 --output-file=tabler-icons-subset.ttf
#define ICON_PLUS  "\xee\xac\x8b"   // U+EB0B
#define ICON_X  "\xee\xad\x95"   // U+EB55
#define ICON_ARROW_BACK  "\xee\xa8\x8c"   // U+EA0C
#define ICON_POINT_FILLED  "\xef\x9a\x98"   // U+F698
#define ICON_BOX  "\xee\xa9\x85"   // U+EA45
#define ICON_CHECK  "\xee\xa9\x9e"   // U+EA5E
#define ICON_GRIP_HORIZONTAL  "\xee\xb0\x80"   // U+EC00
#define ICON_EYE  "\xee\xaa\x9a"   // U+EA9A
#define ICON_EYE_CLOSED  "\xef\x9f\xac"   // U+F7EC
#define ICON_COMPONENTS  "\xee\xbe\xa5"   // U+EFA5
#define ICON_COMPONENTS_OFF  "\xef\x83\x96"   // U+F0D6
#define ICON_COPY_OFF  "\xef\x83\x98"   // U+F0D8
#define ICON_COPY_PLUS  "\xef\xb6\xae"   // U+FDAE
#define ICON_DOTS_VERTICAL  "\xee\xaa\x94"   // U+EA94
#define ICON_REORDER  "\xef\xb0\x95"   // U+FC15
#define ICON_ALERT_TRIANGLE_FILLED  "\xef\x9b\xb0"   // U+F6F0

#define ARRAY_HEADER_BG_COLOR			IM_COL32(255, 255, 255, 32)
#define ARRAY_HEADER_BG_COLOR_ACTIVE	IM_COL32(255, 255, 255, 64)

#define OVERRIDE_COLOR				IM_COL32(68, 164, 248, 255)
#define OVERRIDE_COLOR_DIM			IM_COL32(68, 164, 248, 128)
#define OVERRIDE_COLOR_REMOVE		IM_COL32(224, 87, 87, 255)


float get_override_marker_width(void);
float get_override_marker_space(void);

static inline ImFontConfig ImFontConfig_Default(void)
{
	return (ImFontConfig) {
		.FontDataOwnedByAtlas = true,
		.ExtraSizeScale = 1.0f,
		.GlyphOffset.y = 1.f,
		.GlyphMaxAdvanceX = FLT_MAX,
		.RasterizerMultiply = 1.0f,
		.RasterizerDensity = 1.0f,
	};
}

void setup_imgui_style(ImGuiStyle* style, float scale);

static inline int ImMini(int lhs, int rhs) { return lhs < rhs ? lhs : rhs; }
static inline int ImMaxi(int lhs, int rhs) { return lhs >= rhs ? lhs : rhs; }
static inline float ImMinf(float lhs, float rhs) { return lhs < rhs ? lhs : rhs; }
static inline float ImMaxf(float lhs, float rhs) { return lhs >= rhs ? lhs : rhs; }

//
// Fantasy ImGui API
//

static inline ImVec4 ImGui_GetColorVec4U32(ImU32 c)
{
	return (ImVec4) {
		((c >> IM_COL32_R_SHIFT) & 255) / 255.0f,
		((c >> IM_COL32_G_SHIFT) & 255) / 255.0f,
		((c >> IM_COL32_B_SHIFT) & 255) / 255.0f,
		((c >> IM_COL32_A_SHIFT) & 255) / 255.0f,
	};
}

ImVec2 ImGui_MeasureIconButton(void);
bool ImGui_IconButton(const char* icon);
bool ImGui_IconButtonEx(const char* icon, bool is_enabled);
bool ImGui_IconButtonColored(const char* icon, ImU32 color);
bool ImGui_IconButtonColoredEx(const char* icon, ImU32 color, bool is_enabled);

ImVec2 ImGui_MeasureIcon(void);
void ImGui_Icon(const char* icon);
void ImGui_IconColored(const char* icon, ImU32 color);

void ImGui_SamelineWrapped(float item_width);

ImVec2 ImGui_MeasureFrame(float width_mult);
ImVec2 ImGui_MeasureTextUnformatted(const char* text);
ImVec2 ImGui_MeasureText(const char* fmt, ...);

#define ImGui_DefaultRowHeight (-1.f)

ImRect ImGui_GetItemRect(void);
ImRect ImGui_GetItemContentRect(void);
ImRect ImGui_GetRowRect(float height);
void ImGui_SetNextItemRect(ImVec2 bb_min, ImVec2 bb_max);


//
// Pack Layout (RectCut)
//

typedef enum {
	ImGuiPack_Start,
	ImGuiPack_End,
} ImGuiLayoutPack;

typedef enum {
	ImGuiAlign_Start,
	ImGuiAlign_Center,
	ImGuiAlign_End,
} ImGuiLayoutAlign;

#define ImGui_DefaultSpacing (-1.f)

void ImGui_BeginPack(ImRect rect);
void ImGui_PackAdvance(ImGuiLayoutPack pack, float spacing);
ImRect ImGui_PackNextSlot(ImVec2 size, ImGuiLayoutPack pack, ImGuiLayoutAlign align);
ImRect ImGui_PackNextSlotEx(ImVec2 size, ImGuiLayoutPack pack, ImGuiLayoutAlign align, float spacing_after);
ImRect ImGui_PackNextSlotPct(float width_percent, float height, ImGuiLayoutPack pack, ImGuiLayoutAlign align);
ImRect ImGui_PackNextSlotPctEx(float width_percent, float height, ImGuiLayoutPack pack, ImGuiLayoutAlign align, float spacing_after);
ImRect ImGui_EndPack(void);


//
// Custom helpers and widgets
//

static inline float calc_color_value(ImU32 color)
{
	ImVec4 c = ImGui_GetColorVec4U32(color);
	return c.x * 0.2126f + c.y * 0.7152f + c.z * 0.0722f;
}

ImVec2 measure_override_marker(void);
void override_marker(bool is_overridden, bool has_overrides);
bool item_override_marker(bool is_override);
bool override_marker_box(ImRect rect, bool is_override);

enum {
	ARRAY_HEADER_IS_MODIFIED = 1<<0,
	ARRAY_HEADER_HAS_MODIFIED_ITEMS = 1<<1,
	ARRAY_HEADER_HAS_REMOVED_ITEMS = 1<<2,
	ARRAY_HEADER_HAS_REVERT = 1<<3,
	ARRAY_HEADER_ALLOW_REVERT = 1<<4,
	ARRAY_HEADER_ALLOW_ADD = 1<<5,
	ARRAY_HEADER_ALLOW_INHERIT = 1<<6,
};

typedef struct array_header_state {
	bool is_open;
	bool is_tooltip_hovered;
	bool add_is_clicked;
	bool add_is_tooltip_hovered;
	bool inherit_is_clicked;
	bool inherit_is_tooltip_hovered;
	bool removed_is_tooltip_hovered;
	bool revert_is_clicked;
	bool revert_is_tooltip_hovered;
	ImVec2 menu_pos;
} array_header_state_t;

array_header_state_t edit_array_header(const char* label, int32_t item_count, uint32_t flags);
