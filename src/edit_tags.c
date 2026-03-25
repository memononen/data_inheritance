
#include "edit_tags.h"

#include <stdbool.h>
#include <stdint.h>

#include "dcimgui.h"
#include "dcimgui_internal.h"
#include "imgui_utils.h"

static const tag_desc_t g_tag_descs[] = {
	{"Boiled", IM_COL32(240,195,169,255), 11 },
	{"Baked", IM_COL32(240,195,169,255), 12 },
	{"Fried", IM_COL32(240,195,169,255), 13 },

	{"Banana", IM_COL32(246,235,17,255), 21 },
	{"Kiwi", IM_COL32(169,192,0,255), 22 },
	{"Blueberry", IM_COL32(90,90,155,255), 23 },
	{"Cherry", IM_COL32(157,13,13,255), 24 },

	{"Smoothie", IM_COL32(205,216,235,255), 31 },
	{"Muffin", IM_COL32(205,216,235,255), 32 },
	{"Pudding", IM_COL32(205,216,235,255), 33 },
	{"Juice", IM_COL32(205,216,235,255), 34 },

	{"Death", IM_COL32(44,62,75,255), 41 },
	{"Warning", IM_COL32(255,211,33,255), 42 },
	{"Error", IM_COL32(244,73,51,255), 43 },
	{"Notice", IM_COL32(163,215,250,255), 44 },
};
static const int32_t g_tag_descs_count = IM_COUNTOF(g_tag_descs);


const tag_desc_t* get_tag_by_id(const tag_desc_t* tag_descs, int32_t tag_descs_count, tag_t tag_id)
{
	for (int32_t i = 0; i < tag_descs_count; i++)
		if (tag_descs[i].id == tag_id)
			return &tag_descs[i];
	return NULL;
}

int32_t tags_index_of(const tag_container_t* tags, tag_t tag_id)
{
	for (int32_t i = 0; i < tags->tags_count; i++)
		if (tags->tags[i] == tag_id)
			return i;
	return -1;
}

void tags_add(tag_container_t* tags, tag_t tag_id)
{
	int32_t idx = tags_index_of(tags, tag_id);
	if (idx != -1) return;
	if (tags->tags_count >= MAX_TAGS) return;
	tags->tags[tags->tags_count++] = tag_id;
}

bool tags_is_override(tag_container_t* tags, tag_t tag_id)
{
	for (int32_t i = 0; i < tags->overrides_count; i++)
		if (tags->overrides[i] == tag_id)
			return true;
	return false;
}

void tags_toggle_override(tag_container_t* tags, tag_t tag_id)
{
	int32_t idx = -1;
	for (int32_t i = 0; i < tags->overrides_count; i++) {
		if (tags->overrides[i] == tag_id) {
			idx = i;
			break;
		}
	}
	if (idx == -1) {
		if (tags->overrides_count >= MAX_TAGS) return;
		tags->overrides[tags->overrides_count++] = tag_id;
	} else {
		tags->overrides_count--;
		for (int32_t i = idx; i < tags->overrides_count; i++)
			tags->overrides[i] = tags->overrides[i + 1];
	}
}

void tags_mark_override(tag_container_t* tags, tag_t tag_id)
{
	if (tags_is_override(tags, tag_id))
		return;
	if (tags->overrides_count >= MAX_TAGS) return;
	tags->overrides[tags->overrides_count++] = tag_id;
}

void tags_clear_override(tag_container_t* tags, tag_t tag_id)
{
	int32_t idx = -1;
	for (int32_t i = 0; i < tags->overrides_count; i++) {
		if (tags->overrides[i] == tag_id) {
			idx = i;
			break;
		}
	}
	if (idx == -1) return;

	tags->overrides_count--;
	for (int32_t i = idx; i < tags->overrides_count; i++)
		tags->overrides[i] = tags->overrides[i + 1];
}

void tags_remove_at(tag_container_t* tags, tag_t idx)
{
	tags->tags_count--;
	for (int32_t i = idx; i < tags->tags_count; i++)
		tags->tags[i] = tags->tags[i + 1];
}

void tags_remove(tag_container_t* tags, tag_t tag_id)
{
	int32_t idx = tags_index_of(tags, tag_id);
	if (idx == -1)
		return;
	tags_remove_at(tags, idx);
}

void tags_toggle(tag_container_t* tags, tag_t tag_id)
{
	const int32_t idx = tags_index_of(tags, tag_id);
	if (idx == -1)
		tags_add(tags, tag_id);
	else
		tags_remove_at(tags, idx);
}

bool tags_has_overrides(const tag_container_t* tags)
{
	return tags->overrides_count > 0;
}

int32_t tags_get_discarded_count(const tag_container_t* tags)
{
	int32_t removed_count = 0;
	for (int32_t i = 0; i < tags->overrides_count; i++) {
		if (tags_index_of(tags, tags->overrides[i]) == -1)
			removed_count++;
	}
	return removed_count;
}

int tags_cmp(const void* a, const void* b)
{
	return *(int32_t*)a - *(int32_t*)b;
}

void tags_sort(tag_container_t* tags)
{
	if (tags->tags_count > 1)
		qsort(tags->tags, tags->tags_count, sizeof(int32_t), tags_cmp);
}

void tags_update_inherited_data(const tag_container_t* tags_base, tag_container_t* tags_derived)
{
	tag_t results[MAX_TAGS] = {0};
	int32_t results_count = 0;

	// Overridden tags from derived
	for (int32_t i = 0; i < tags_derived->tags_count; i++) {
		const tag_t tag_id = tags_derived->tags[i];
		if (tags_is_override(tags_derived, tag_id)) {
			assert(results_count < MAX_TAGS);
			results[results_count++] = tag_id;
		}
	}

	// Non-overridden tags from base
	for (int32_t i = 0; i < tags_base->tags_count; i++) {
		const tag_t tag_id = tags_base->tags[i];
		if (!tags_is_override(tags_derived, tag_id)) {
			assert(results_count < MAX_TAGS);
			results[results_count++] = tag_id;
		}
	}

	// Store result in derived and sort.
	for (int32_t i = 0; i < results_count; i++)
		tags_derived->tags[i] = results[i];
	tags_derived->tags_count = results_count;

	tags_sort(tags_derived);
}

//
// UI
//

bool tag_picker(tag_container_t* tags, const tag_desc_t* tag_descs, int32_t tag_descs_count, bool is_derived)
{
	const float frame_h = ImGui_GetFrameHeight();

	bool changed = false;

	if (ImGui_BeginChild("tag_list", (ImVec2){300,400}, 0, 0)) {
		for (int32_t i = 0; i < tag_descs_count; i++) {
			const tag_desc_t* desc = &tag_descs[i];

			ImGui_PushIDInt(desc->id);

			bool is_selected = tags_index_of(tags, desc->id) != -1;
			bool is_override = tags_is_override(tags, desc->id);

			bool item_toggled = ImGui_SelectableEx("##item", false, ImGuiSelectableFlags_AllowOverlap, (ImVec2){0, frame_h});

			if (is_selected)
				ImGui_SetItemDefaultFocus();

			ImGui_BeginPack(ImGui_GetItemContentRect());

			// Add a bit extra space for the override marker.
			ImGui_PackAdvance(ImGuiPack_Start, get_override_marker_width());

			ImGui_PushStyleColor(ImGuiCol_FrameBg, desc->color);
			ImGui_PushStyleColor(ImGuiCol_FrameBgActive, desc->color);
			ImGui_PushStyleColor(ImGuiCol_FrameBgHovered, desc->color);
			ImGui_PushStyleColor(ImGuiCol_CheckMark, calc_color_value(desc->color) < 0.7f ? IM_COL32(255,255,255,255) : IM_COL32(0,0,0,255));

			ImGui_PackNextSlot(ImGui_MeasureFrame(1.f), ImGuiPack_Start, ImGuiAlign_Center);
			bool is_selected_dummy = is_selected;
			if (ImGui_Checkbox("##selected", &is_selected_dummy)) // Expecting that the button will not get activated, since the selectable does not allow overlap.
				item_toggled = true;

			ImGui_PopStyleColorEx(4);

			// Override marker
			if (is_derived)
				override_marker_overlay(ImGui_GetItemRect(), is_override, false);

			ImGui_PackNextSlot(ImGui_MeasureTextUnformatted(desc->name), ImGuiPack_Start, ImGuiAlign_Center);
			ImGui_AlignTextToFramePadding();
			ImGui_TextUnformatted(desc->name);

			// Revert
			if (is_derived) {
				ImGui_PackNextSlot(ImGui_MeasureIconButton(), ImGuiPack_End, ImGuiAlign_Center);
				if (ImGui_IconButtonEx(ICON_ARROW_BACK, is_override)) {
					tags_clear_override(tags, desc->id);
					changed = true;
				}
			}

			ImGui_EndPack();

			ImGui_PopID();

			if (item_toggled) {
				if (is_derived)
					tags_mark_override(tags, desc->id);
				tags_toggle(tags, desc->id);
			}

			changed |= item_toggled;

		}

		ImGui_EndChild();
	}

	if (changed)
		tags_sort(tags);

	return changed;
}

typedef struct {
	bool tag_clicked;
	bool remove_clicked;
	bool revert_clicked;
} tag_chip_result_t;

typedef enum {
	TAG_CHIP_ALLOW_CLICK = 1<<0,
	TAG_CHIP_ALLOW_REMOVE = 1<<1,
	TAG_CHIP_ALLOW_OVERRIDE = 1<<2,
	TAG_CHIP_IS_OVERRIDE = 1<<3,
	TAG_CHIP_WRAP_LINE = 1<<4,
} tag_chip_flags_t;

tag_chip_result_t tag_chip(const char* label, ImU32 tag_color, uint32_t flags)
{
	tag_chip_result_t result = {0};

	ImGui_PushID(label);

	const char* remove_str = ICON_X;
	const ImGuiStyle* style = ImGui_GetStyle();
	const ImVec2 label_size = ImGui_CalcTextSizeEx(label, NULL, true, -1);
	const float remove_button_width = ImGui_GetFrameHeight();
	float button_width = label_size.x + style->FramePadding.x * 2.0f;
	if (flags & TAG_CHIP_ALLOW_REMOVE)
		button_width += remove_button_width;
	if (flags & TAG_CHIP_IS_OVERRIDE)
		button_width += get_override_marker_width() + get_override_marker_space();
	ImVec2 button_size = ImGui_CalcItemSize((ImVec2){0,0}, button_width, label_size.y + style->FramePadding.y * 2.0f);

	if (flags & TAG_CHIP_WRAP_LINE)
		ImGui_SamelineWrapped(button_size.x);

	const float rounding = style->FrameRounding;

	// Label
	result.tag_clicked = ImGui_InvisibleButton("tag", button_size, ImGuiButtonFlags_MouseButtonLeft | (unsigned)ImGuiButtonFlags_AllowOverlap);
	ImRect tag_rect = { .Min = ImGui_GetItemRectMin(), .Max = ImGui_GetItemRectMax() };
	bool tag_hovered = ImGui_IsItemHovered(0);
	bool tag_active = ImGui_IsItemActive();

	// Revert context menu.
	if (flags & TAG_CHIP_ALLOW_OVERRIDE) {
		if (ImGui_BeginPopupContextItemEx("tag", 0)) {
			ImGui_BeginDisabled(!(flags & TAG_CHIP_IS_OVERRIDE));
			if (ImGui_MenuItemWithIcon("Revert Change", ICON_ARROW_BACK)) {
				result.revert_clicked = true;
			}
			ImGui_EndDisabled();
			ImGui_EndPopup();
		}
	}

	ImVec2 remove_button_pos = { .x = tag_rect.Max.x - remove_button_width, .y = tag_rect.Min.y };
	ImVec2 remove_button_size = { .x = remove_button_width, .y = tag_rect.Max.y - tag_rect.Min.y };

	// Remove button
	bool remove_hovered = false;
	bool remove_active = false;

	if (flags & TAG_CHIP_ALLOW_REMOVE) {
		ImGui_SetCursorScreenPos(remove_button_pos);
		result.remove_clicked = ImGui_InvisibleButton("tag_remove", remove_button_size, ImGuiButtonFlags_MouseButtonLeft | (unsigned)ImGuiButtonFlags_AllowOverlap);
		remove_hovered = ImGui_IsItemHovered(0);
		remove_active = ImGui_IsItemActive();
	}

	ImDrawList* draw_list = ImGui_GetWindowDrawList();

	bool tag_color_is_dark = calc_color_value(tag_color) < 0.7f;
	ImU32 label_color = tag_color_is_dark ? IM_COL32(255,255,255,255) : IM_COL32(0,0,0,255);

	// Override marker.
	override_marker_overlay(tag_rect, flags & TAG_CHIP_IS_OVERRIDE, false);

	// Tag Body
	tag_color = ImGui_GetColorU32ImU32Ex(tag_color, (flags & TAG_CHIP_ALLOW_CLICK) && (tag_active | tag_hovered) ? 1.f : 0.85f);
	ImDrawList_AddRectFilledEx(draw_list, tag_rect.Min, tag_rect.Max, tag_color, rounding, ImDrawFlags_RoundCornersAll);

	// Remove button hover
	if (remove_hovered || remove_active) {
		ImVec2 remove_bmin = { remove_button_pos.x, remove_button_pos.y };
		ImVec2 remove_bmax = { remove_button_pos.x + remove_button_size.x, remove_button_pos.y + remove_button_size.y };
		ImU32 remove_color = ImGui_GetColorU32ImU32Ex(label_color, remove_active ? 0.5f : 0.25f);
		ImDrawList_AddRectFilledEx(draw_list, remove_bmin, remove_bmax, remove_color, rounding, ImDrawFlags_RoundCornersRight);
	}

	// Label
	label_color = ImGui_GetColorU32ImU32Ex(label_color, tag_active ? 1.0f : 0.85f);
	ImVec2 label_pos = {
		IM_ROUND(tag_rect.Min.x + style->FramePadding.x),
		IM_ROUND(tag_rect.Min.y + (tag_rect.Max.y - tag_rect.Min.y) * 0.5f - label_size.y * 0.5f),
	};
	ImDrawList_AddText(draw_list, label_pos, label_color, label);

	// Remove label
	if (flags & TAG_CHIP_ALLOW_REMOVE) {
		ImU32 remove_color = tag_color_is_dark ? IM_COL32(255,255,255,255) : IM_COL32(0,0,0,255);
		if (remove_active)
			remove_color = ImGui_GetColorU32ImU32Ex(remove_color, 1.0f);
		else
			remove_color = ImGui_GetColorU32ImU32Ex(remove_color, 0.75f);

		const ImVec2 remove_size = ImGui_CalcTextSizeEx(remove_str, NULL, true, -1);

		ImVec2 remove_pos = {
			IM_ROUND(remove_button_pos.x + remove_button_size.x * 0.5f - remove_size.x * 0.5f),
			IM_ROUND(remove_button_pos.y + remove_button_size.y * 0.5f - remove_size.y * 0.5f + 1),
		};

		ImDrawList_AddText(draw_list, remove_pos, remove_color, remove_str);
	}

	ImGui_PopID();

	return result;
}


bool edit_tags(tag_container_t* tags, bool is_derived)
{
	bool changed = false;

	int32_t discarded_count = tags_get_discarded_count(tags);

	uint32_t header_flags = ARRAY_HEADER_ALLOW_ADD;
	if (is_derived) {
		header_flags |= ARRAY_HEADER_HAS_REVERT;

		const bool has_overrides = tags_has_overrides(tags);
		if (has_overrides)
			header_flags |= ARRAY_HEADER_HAS_MODIFIED_ITEMS;
		if (has_overrides)
			header_flags |= ARRAY_HEADER_ALLOW_REVERT;
		if (discarded_count > 0)
			header_flags |= ARRAY_HEADER_HAS_REMOVED_ITEMS;
	}

	// Header
	array_header_state_t header = edit_array_header("Tags", tags->tags_count, header_flags);

	// Add
	if (header.add_is_clicked) {
		ImGui_OpenPopup("tag_popup", 0);
		ImGui_SetNextWindowPos(header.menu_pos, ImGuiCond_Always);
	}
	if (ImGui_BeginPopup("tag_popup", 0)) {
		changed |= tag_picker(tags, g_tag_descs, g_tag_descs_count, is_derived);
		ImGui_EndPopup();
	}
	if (header.add_is_tooltip_hovered) {
		ImGui_SetTooltip("Add or remove tags.");
	}

	// Revert all
	if (header.revert_is_clicked && is_derived) {
		ImGui_OpenPopup("revert_all_menu", 0);
	}
	if (header.revert_is_tooltip_hovered) {
		if (ImGui_BeginTooltip()) {
			ImGui_Text("Revert Changes.");
			ImGui_Text("%d removed items.", discarded_count);
			ImGui_EndTooltip();
		}
	}
	if (ImGui_BeginPopup("revert_all_menu", 0)) {

		if (ImGui_MenuItemWithIcon("Revert All", ICON_ARROW_BACK)) {
			tags->overrides_count = 0;
			changed = true;
		}

		ImGui_BeginDisabled(discarded_count == 0);
		if (ImGui_MenuItemWithIcon("Revert All Removed", "")) {
			for (int32_t i = 0; i < tags->overrides_count; i++) {
				if (tags_index_of(tags, tags->overrides[i]) == -1) {
					tags_clear_override(tags, tags->overrides[i]);
					i--;
				}
			}
			changed = true;
		}
		ImGui_EndDisabled();

		if (discarded_count > 0) {
			if (ImGui_BeginMenu("Revert Removed")) {
				for (int32_t i = 0; i < tags->overrides_count; i++) {
					if (tags_index_of(tags, tags->overrides[i]) == -1) {
						const tag_desc_t* cur_desc = get_tag_by_id(g_tag_descs, g_tag_descs_count, tags->overrides[i]);
						const char* tag_name = cur_desc ? cur_desc->name : "None";
						if (ImGui_MenuItem(tag_name)) {
							tags_toggle_override(tags, tags->overrides[i]);
							changed = true;
						}
					}
				}
				ImGui_EndMenu();
			}
		}

		ImGui_EndPopup();
	}


	// Removed items
	if (header.removed_is_tooltip_hovered) {
		if (ImGui_BeginTooltip()) {
			ImGui_Text("%d items removed.", discarded_count);
			for (int32_t i = 0; i < tags->overrides_count; i++) {
				if (tags_index_of(tags, tags->overrides[i]) == -1) {
					const tag_desc_t* cur_desc = get_tag_by_id(g_tag_descs, g_tag_descs_count, tags->overrides[i]);
					const char* tag_name = cur_desc ? cur_desc->name : "None";
					ImGui_TextUnformatted(tag_name);
				}
			}
			ImGui_EndTooltip();
		}
	}

	if (header.is_open) {
		// Tags
		for (int32_t i = 0; i < tags->tags_count; i++) {
			ImGui_PushIDInt(i);

			const tag_desc_t* cur_desc = get_tag_by_id(g_tag_descs, g_tag_descs_count, tags->tags[i]);
			const char* tag_name = cur_desc ? cur_desc->name : "None";
			ImU32 tag_color = cur_desc ? cur_desc->color : IM_COL32(255,255,255,255);

			uint32_t flags = TAG_CHIP_ALLOW_REMOVE;
			if (is_derived) {
				flags |= TAG_CHIP_ALLOW_OVERRIDE;
				if (tags_is_override(tags, tags->tags[i]))
					flags |= TAG_CHIP_IS_OVERRIDE;
			}
			if (i > 0)
				flags |= TAG_CHIP_WRAP_LINE;

			tag_chip_result_t act = tag_chip(tag_name, tag_color, flags);
			if (act.remove_clicked) {
				if (is_derived)
					tags_toggle_override(tags, cur_desc->id);
				tags_remove(tags, cur_desc->id);
				changed = true;
			}
			if (act.revert_clicked) {
				tags_toggle_override(tags, cur_desc->id);
				changed = true;
			}

			ImGui_PopID();
		}
	}

	tags->has_changes |= changed;

	return changed;
}
