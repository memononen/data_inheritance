#include "edit_gradient.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "dcimgui.h"
#include "dcimgui_internal.h"
#include "imgui_utils.h"
#include "utils.h"

uid_t gradient_add(gradient_t* grad, float t, ImVec4 color)
{
	if (grad->stops_count >= MAX_COLOR_STOPS) return 0;
	int32_t idx = grad->stops_count++;
	grad->stops[idx].pos = t;
	grad->stops[idx].color = color;
	grad->stops[idx].id = gen_id();
	return grad->stops[idx].id;
}

void gradient_insert_at(gradient_t* grad, int32_t idx, color_stop_t new_stop)
{
	if (grad->stops_count >= MAX_COLOR_STOPS)
		return;
	if (idx == -1)
		idx = grad->stops_count;

	for (int32_t i = grad->stops_count; i > idx; i--)
		grad->stops[idx] = grad->stops[idx-1];

	grad->stops[idx] = new_stop;
	grad->stops_count++;
}

void gradient_remove_at(gradient_t* grad, int32_t idx)
{
	grad->stops_count--;
	for (int32_t i = idx; i < grad->stops_count; i++)
		grad->stops[i] = grad->stops[i+1];
}

int32_t gradient_index_of(const gradient_t* grad, uid_t id)
{
	for (int32_t i = 0; i < grad->stops_count; i++)
		if (grad->stops[i].id == id)
			return i;
	return INVALID_INDEX;
}

void gradient_remove(gradient_t* grad, uid_t id)
{
	int32_t idx = gradient_index_of(grad, id);
	if (idx == INVALID_INDEX) return;
	gradient_remove_at(grad, idx);
}

bool gradient_is_override(gradient_t* grad, uid_t id)
{
	for (int32_t i = 0; i < grad->stops_count; i++)
		if (grad->stops[i].id == id)
			return grad->stops[i].is_inserted;
	for (int32_t i = 0; i < grad->discarded_count; i++)
		if (grad->discarded[i] == id)
			return true;
	return false;
}

bool gradient_is_discarded(gradient_t* grad, uid_t id)
{
	for (int32_t i = 0; i < grad->discarded_count; i++)
		if (grad->discarded[i] == id)
			return true;
	return false;
}


void gradient_mark_discarded(gradient_t* grad, uid_t id)
{
	if (id == INVALID_ID) return;
	for (int32_t i = 0; i < grad->discarded_count; i++)
		if (grad->discarded[i] == id)
			return;
	if (grad->discarded_count >= MAX_COLOR_STOPS) return;
	grad->discarded[grad->discarded_count++] = id;
}

void color_stop_clear_overrides(color_stop_t* stop);

void gradient_remove_discard_at(gradient_t* grad, int32_t idx)
{
	grad->discarded_count--;
	for (int32_t i = idx; i < grad->discarded_count; i++)
		grad->discarded[i] = grad->discarded[i+1];
}

void gradient_clear_override(gradient_t* grad, uid_t id)
{
	for (int32_t i = 0; i < grad->stops_count; i++) {
		if (grad->stops[i].id == id) {
			if (grad->stops[i].is_inserted)
				gradient_remove_at(grad, i);
			else
				color_stop_clear_overrides(&grad->stops[i]);
			return;
		}
	}
	int32_t idx = INVALID_INDEX;
	for (int32_t i = 0; i < grad->discarded_count; i++) {
		if (grad->discarded[i] == id) {
			idx = i;
			break;
		}
	}
	if (idx == INVALID_INDEX) return;
	gradient_remove_discard_at(grad, idx);
}

void gradient_clear_all_overrides(gradient_t* grad)
{
	for (int32_t i = 0; i < grad->stops_count; i++) {
		if (grad->stops[i].is_inserted) {
			gradient_remove_at(grad, i);
			i--;
		} else {
			color_stop_clear_overrides(&grad->stops[i]);
		}
	}
	grad->discarded_count = 0;
}

void gradient_clear_discareded(gradient_t* grad)
{
	grad->discarded_count = 0;
}

bool color_stop_has_overrides(const color_stop_t* stop);

bool gradient_has_overrides(gradient_t* grad)
{
	if (grad->discarded_count > 0)
		return true;
	for (int32_t i = 0; i < grad->stops_count; i++) {
		if (grad->stops[i].is_inserted)
			return true;
		if (color_stop_has_overrides(&grad->stops[i]))
			return true;
	}
	return false;
}

int32_t gradient_get_discarded_count(gradient_t* grad)
{
	return grad->discarded_count;
}



bool color_stop_has_overrides(const color_stop_t* stop)
{
	return stop->override_color || stop->override_pos;
}

void color_stop_clear_overrides(color_stop_t* stop)
{
	stop->override_pos = false;
	stop->override_color = false;
}



static int color_stop_cmp(const void* a, const void* b)
{
	const color_stop_t* sa = a;
	const color_stop_t* sb = b;
	if (sa->pos < sb->pos) return -1;
	if (sa->pos > sb->pos) return 1;
	return 0;
}

static void gradient_sort(gradient_t* grad)
{
	qsort(grad->stops, grad->stops_count, sizeof(color_stop_t), color_stop_cmp);
}

ImVec4 interpolate(const color_stop_t* stops, int32_t count, float t)
{
	if (count == 0)
		return (ImVec4){0,0,0,1};
	if (count == 1)
		return stops[0].color;
	if (t <= stops[0].pos)
		return stops[0].color;
	if (t >= stops[count-1].pos)
		return stops[count-1].color;

	for (int32_t i = 1; i < count; i++) {
		if (t < stops[i].pos) {
			float t0 = t - stops[i-1].pos;
			float range = stops[i].pos - stops[i-1].pos;
			float u = range > 1e-6f ? clampf(t0 / range, 0.f, 1.f) : 0.f;
			return (ImVec4) {
				.x = stops[i-1].color.x + (stops[i].color.x - stops[i-1].color.x) * u,
				.y = stops[i-1].color.y + (stops[i].color.y - stops[i-1].color.y) * u,
				.z = stops[i-1].color.z + (stops[i].color.z - stops[i-1].color.z) * u,
				.w = stops[i-1].color.w + (stops[i].color.w - stops[i-1].color.w) * u,
			};
		}
	}
	return stops[count-1].color;
}


void gradient_update_inherited_data(const gradient_t* base_grad, gradient_t* derived_grad)
{
	// Take copy of the derived data before modification.
	color_stop_t result_stops[MAX_COLOR_STOPS];
	int32_t result_stops_count = 0;

	// Keep stops from derived that are overridden.
	for (int32_t i = 0; i < derived_grad->stops_count; i++) {
		const color_stop_t* stop = &derived_grad->stops[i];
		if (stop->is_inserted) {
			assert(result_stops_count < MAX_COLOR_STOPS);
			result_stops[result_stops_count++] = *stop;
		}
	}

	// Keep stops from base which are not overridden, and update property overrides.
	for (int32_t i = 0; i < base_grad->stops_count; i++) {
		const color_stop_t* stop = &base_grad->stops[i];
		// if we have discard override, just skip.
		if (gradient_is_discarded(derived_grad, stop->id))
			continue;
		const int32_t derived_idx = gradient_index_of(derived_grad, stop->id);
		if (derived_idx == INVALID_INDEX) {
			// The item exists only in base, copy over (without base overrides).
			color_stop_t new_stop = {
				.pos = stop->pos,
				.color = stop->color,
				.id = stop->id,
			};
			assert(result_stops_count < MAX_COLOR_STOPS);
			result_stops[result_stops_count++] = new_stop;
		} else {
			// If the item exists both in base and derived, we need to merge properties.
			assert(derived_idx != -1);
			color_stop_t existing_stop = derived_grad->stops[derived_idx];
			assert(!existing_stop.is_inserted);
			// Inherit data
			if (!existing_stop.override_pos)
				existing_stop.pos = stop->pos;
			if (!existing_stop.override_color)
				existing_stop.color = stop->color;
			assert(result_stops_count < MAX_COLOR_STOPS);
			result_stops[result_stops_count++] = existing_stop;
		}
	}

	// Remove discard overrides that do not exists in base anymore
	for (int32_t i = 0; i < derived_grad->discarded_count; i++) {
		if (gradient_index_of(base_grad, derived_grad->discarded[i]) == -1) {
			// Could not find in base, remove.
			gradient_remove_discard_at(derived_grad, i);
			i--;
		}
	}

	// Copy results back to derived.
	derived_grad->stops_count = result_stops_count;
	for (int32_t i = 0; i < result_stops_count; i++)
		derived_grad->stops[i] = result_stops[i];

	gradient_sort(derived_grad);
}


//
// UI
//

static float get_key_width(void)
{
	return IM_TRUNC(ImGui_GetFrameHeight() * 0.5f);
}

typedef struct {
	bool remove_clicked;
	bool changed;
} edit_color_stop_key_result_t;

edit_color_stop_key_result_t edit_color_stop_key(gradient_t* grad, int32_t idx, ImVec2 view_min, ImVec2 view_max, bool is_derived)
{
	edit_color_stop_key_result_t result = {0};
	color_stop_t* stop = &grad->stops[idx];

	ImGuiStyle* style = ImGui_GetStyle();
	ImDrawList* draw_list = ImGui_GetWindowDrawList();
	const float key_width = get_key_width();

	const float min_x = view_min.x;
	const float max_x = view_max.x;
	const float x = min_x + clampf(stop->pos, 0.f, 1.0f) * (max_x - min_x) - key_width/2;

	ImVec2 pos = { .x = x, .y = view_min.y };
	ImVec2 size = { .x = key_width, .y = view_max.y - view_min.y };

	ImGui_SetCursorScreenPos(pos);

	ImGui_InvisibleButton("handle", size, ImGuiButtonFlags_MouseButtonLeft | (unsigned)ImGuiButtonFlags_AllowOverlap);
	ImVec2 bmin = ImGui_GetItemRectMin();
	ImVec2 bmax = ImGui_GetItemRectMax();

	static ImVec2 drag_start_mouse_pos = {0};
	static float drag_start_t = 0.f;
	static bool dragging = false;

	if (ImGui_IsItemActivated()) {
		drag_start_mouse_pos = ImGui_GetMousePos();
		drag_start_t = stop->pos;
	}
	if (ImGui_IsItemActive()) {
		ImVec2 mouse_pos = ImGui_GetMousePos();
		float mouse_dx = mouse_pos.x - drag_start_mouse_pos.x;
		if (fabsf(mouse_dx) > 5.f || ImGui_IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
			dragging = true;
		}
		if (dragging) {
			const float range_x = max_x - min_x;
			if (range_x > 1.f) {
				float dt = mouse_dx / range_x;
				stop->pos = clampf(drag_start_t + dt, 0.f, 1.f);
				if (is_derived && !stop->is_inserted)
					stop->override_pos = true;
			}
		}
	}
	if (ImGui_IsItemDeactivated()) {
		if (dragging) {
			result.changed = true;
		} else {
			if (ImGui_GetIO()->KeyCtrl) {
				// Remove in CTRL+click
				result.remove_clicked = true;
			} else {
				// Color popup
				ImGui_OpenPopup("picker_popup", 0);
			}
		}
		dragging = false;
	}

	bool highlight = ImGui_IsItemHovered(0) || ImGui_IsItemActive();
	ImVec2 center = { (bmin.x + bmax.x) / 2.f, (bmin.y + bmax.y) / 2.f };

	// Diamond shape
	ImDrawList_AddCircleFilled(draw_list, center, key_width*0.5f + 3, IM_COL32(0,0,0,128), 4);
	ImDrawList_AddCircleFilled(draw_list, center, key_width*0.5f, highlight ? IM_COL32(255,255,255,255) : IM_COL32(192,192,192,255), 4);

	if (is_derived) {
		if (stop->is_inserted || color_stop_has_overrides(stop)) {
			ImU32 marker_color = stop->is_inserted ? OVERRIDE_COLOR : OVERRIDE_COLOR_DIM;
			ImVec2 marker_bmin = { bmin.x, bmin.y - get_override_marker_space() - get_override_marker_width() };
			ImVec2 marker_bmax = { bmax.x, bmin.y - get_override_marker_space() };
			ImDrawList_AddRectFilledEx(draw_list, marker_bmin, marker_bmax, marker_color, style->FrameRounding, ImDrawFlags_RoundCornersTop);
		}
	}

	return result;
}

bool edit_gradient(gradient_t* grad, const gradient_t* base_grad)
{
	const bool is_derived = base_grad != NULL;
	bool changed = false;
	command_t command = {0};
	int32_t discarded_count = gradient_get_discarded_count(grad);
	const bool has_overrides = gradient_has_overrides(grad);

	// Sorted copy of the stops, allows stop positions to be adjusted without having to sort stops.
	color_stop_t stops[MAX_COLOR_STOPS];
	for (int32_t i = 0; i < grad->stops_count; i++)
		stops[i] = grad->stops[i];
	qsort(stops, grad->stops_count, sizeof(color_stop_t), color_stop_cmp);

	uint32_t header_flags = ARRAY_HEADER_ALLOW_ADD;
	if (is_derived) {
		header_flags |= ARRAY_HEADER_HAS_REVERT;
		if (has_overrides)
			header_flags |= ARRAY_HEADER_HAS_MODIFIED_ITEMS;
		if (has_overrides)
			header_flags |= ARRAY_HEADER_ALLOW_REVERT;
		if (discarded_count > 0)
			header_flags |= ARRAY_HEADER_HAS_REMOVED_ITEMS;
	}

	// Header
	array_header_state_t header = edit_array_header("Gradient", grad->stops_count, header_flags);

	if (header.add_is_clicked) {
		command = command_make_insert_at_pos(-1.f);
	}
	if (header.add_is_tooltip_hovered)
		ImGui_SetTooltip("Add new color stop.");

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
			command = command_make_revert_all();
		}

		ImGui_BeginDisabled(discarded_count == 0);
		if (ImGui_MenuItemWithIcon("Revert All Removed", "")) {
			command = command_make_revert_all_discarded();
		}
		ImGui_EndDisabled();

		if (discarded_count > 0) {
			if (ImGui_BeginMenu("Revert Removed")) {
				for (int32_t i = 0; i < grad->discarded_count; i++) {
					const int32_t base_idx = gradient_index_of(base_grad, grad->discarded[i]);
					const color_stop_t* base_item = base_idx != INVALID_INDEX ? &base_grad->stops[base_idx] : NULL;
					if (base_item) {
						char label[64];
						snprintf(label, sizeof(label), "Color Stop at %.3f", base_item->pos);
						if (ImGui_MenuItem(label)) {
							command = command_make_revert_discarded(grad->discarded[i]);
						}

					}
				}
				ImGui_EndMenu();
			}
		}

		ImGui_EndPopup();
	}

	// Removed
	if (header.removed_is_tooltip_hovered) {
		if (ImGui_BeginTooltip()) {
			ImGui_Text("%d items removed.", grad->discarded_count);
			for (int32_t i = 0; i < grad->discarded_count; i++) {
				const int32_t base_idx = gradient_index_of(base_grad, grad->discarded[i]);
				const color_stop_t* base_item = base_idx != INVALID_INDEX ? &base_grad->stops[base_idx] : NULL;
				if (base_item) {
					ImGui_Text("Color Stop at %.3f", base_item->pos);
				}
			}
			ImGui_EndTooltip();
		}
	}

	// Gradient
	const float key_width = get_key_width();
	ImVec2 avail = ImGui_GetContentRegionAvail();
	float row_h = ImGui_GetFrameHeight();
	ImGui_InvisibleButton("grad", (ImVec2){avail.x, row_h + get_override_marker_width()}, ImGuiButtonFlags_MouseButtonLeft | (unsigned)ImGuiButtonFlags_AllowOverlap);

	ImVec2 bmin = ImGui_GetItemRectMin();
	ImVec2 bmax = ImGui_GetItemRectMax();
	bmin.x += key_width/2;
	bmin.y += get_override_marker_width();
	bmax.x -= key_width/2;

	ImDrawList* draw_list = ImGui_GetWindowDrawList();

	ImDrawList_AddRectFilled(draw_list, bmin, bmax, IM_COL32(0,0,0,128));

	if (grad->stops_count == 1) {
		ImDrawList_AddRectFilled(draw_list, bmin, bmax, ImGui_GetColorU32ImVec4(stops[0].color));
	} else if (grad->stops_count > 1) {
		float prev_x = bmin.x;
		ImU32 prev_color = ImGui_GetColorU32ImVec4(stops[0].color);
		for (int32_t i = 0; i < grad->stops_count; i++) {
			float x = bmin.x + stops[i].pos * (bmax.x - bmin.x);
			ImU32 color = ImGui_GetColorU32ImVec4(stops[i].color);
			ImDrawList_AddRectFilledMultiColor(draw_list, (ImVec2){ prev_x, bmin.y }, (ImVec2){x, bmax.y}, prev_color, color, color, prev_color);
			prev_x = x;
			prev_color = color;
		}
		ImU32 color = ImGui_GetColorU32ImVec4(stops[grad->stops_count-1].color);
		ImDrawList_AddRectFilledMultiColor(draw_list, (ImVec2){ prev_x, bmin.y }, bmax, prev_color, color, color, prev_color);
	}

	if (header.is_open) {

		// Double click to insert new item.
		if (ImGui_IsItemClicked() && ImGui_IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
			const ImVec2 mouse_pos = ImGui_GetMousePos();
			const float mouse_t = clampf((mouse_pos.x - bmin.x) / (bmax.x - bmin.x), 0.f, 1.f);
			command = command_make_insert_at_pos(mouse_t);
		}

		// Edit color stops on the gradient.
		for (int32_t i = 0; i < grad->stops_count; i++) {
			color_stop_t* stop = &grad->stops[i];
			ImGui_PushIDInt(stop->id);

			edit_color_stop_key_result_t result = edit_color_stop_key(grad, i, bmin, bmax, is_derived);
			if (result.remove_clicked) {
				command = command_make_remove_at(i);
			}
			changed |= result.changed;

			if (ImGui_BeginPopup("picker_popup", 0)) {
				float col[4];
				memcpy(col, &stop->color.x, sizeof(col));
				if (ImGui_ColorPicker4("picker", col, 0, NULL)) {
					memcpy(&stop->color.x, col, sizeof(col));
					if (is_derived && !stop->is_inserted)
						stop->override_color = true;
					changed = true;
				}
				ImGui_EndPopup();
			}

			ImGui_PopID();
		}

		// List of color stops
		for (int32_t i = 0; i < grad->stops_count; i++) {
			color_stop_t* stop = &grad->stops[i];
			ImGui_PushIDInt(stop->id);

			ImRect row_rect = ImGui_GetRowRect(ImGui_DefaultRowHeight);
			ImGui_BeginPack(row_rect);

			const bool item_is_override = stop->is_inserted;
			const bool item_has_overrides = color_stop_has_overrides(stop);

			// Override marker for the whole row
			if (is_derived)
				override_marker_overlay(row_rect, item_is_override, item_has_overrides);

			// Row icon so that we have space for full row indicator and first widget indicator
			ImGui_PackNextSlot(ImGui_MeasureIcon(), ImGuiPack_Start, ImGuiAlign_Center);
			ImGui_IconColored(ICON_KEYFRAME, IM_COL32(255,255,255,128));

			// Revert button
			if (is_derived) {
				ImGui_PackNextSlotEx(ImGui_MeasureIconButton(), ImGuiPack_End, ImGuiAlign_Center, 0.f);
				if (ImGui_IconButtonColoredEx(ICON_ARROW_BACK, IM_COL32(255, 255, 255, 128), item_has_overrides || item_is_override)) {
					if (ImGui_GetIO()->KeyCtrl)
						command = command_make_revert_at(i);
					else
						ImGui_OpenPopup("revert_menu", 0);
				}
				ImGui_SetItemTooltip("Revert changes.");

				if (ImGui_BeginPopup("revert_menu", 0)) {

					if (ImGui_MenuItemWithIconEx("Revert All", ICON_ARROW_BACK, NULL, false, item_has_overrides || item_is_override)) {
						command = command_make_revert_at(i);
					}
					if (ImGui_MenuItemWithIconEx("Revert property Position", ICON_EDIT, NULL, false, stop->override_pos)) {
						stop->override_pos = false;
						changed = true;
					}
					if (ImGui_MenuItemWithIconEx("Revert property Color", ICON_EDIT, NULL, false, stop->override_color)) {
						stop->override_color = false;
						changed = true;
					}
					ImGui_EndPopup();
				}
			}

			// Remove
			ImGui_PackNextSlotEx(ImGui_MeasureIconButton(), ImGuiPack_End, ImGuiAlign_Center, 0.f);
			if (ImGui_IconButtonColored(ICON_X, IM_COL32(255, 255, 255, 128))) {
				command = command_make_remove_at(i);
			}
			ImGui_SetItemTooltip("Remove color stop.");


			// Stop position
			ImGui_PackNextSlot(ImGui_MeasureFrame(3.f), ImGuiPack_Start, ImGuiAlign_Center);
			if (ImGui_InputFloat("##pos", &stop->pos)) {
				stop->pos = clampf(stop->pos, 0.f, 1.f);
				if (is_derived && !stop->is_inserted)
					stop->override_pos = true;
				changed = true;
			}
			if (is_derived)
				override_marker_overlay(ImGui_GetItemRect(), stop->override_pos, false);

			// Color
			ImGui_PackNextSlotPct(1.f, row_h, ImGuiPack_Start, ImGuiAlign_Center);
			float col[4];
			memcpy(col, &stop->color.x, sizeof(col));
			if (ImGui_ColorEdit4("##color", col, ImGuiColorEditFlags_DisplayHex)) {
				memcpy(&stop->color.x, col, sizeof(col));
				if (is_derived && !stop->is_inserted)
					stop->override_color = true;
				changed = true;
			}
			if (is_derived)
				override_marker_overlay(ImGui_GetItemRect(), stop->override_color, false);

			ImGui_EndPack();

			ImGui_PopID();
		}
	}

	if (command.type == COMMAND_INSERT_AT) {
		color_stop_t new_stop = { .id = gen_id() };
		if (command.pos < 0.f) {
			// From add button
			if (grad->stops_count == 0) {
				new_stop.pos = 0.f;
				new_stop.color = (ImVec4){1,1,1,1};
			} else if (grad->stops_count == 1) {
				new_stop.pos = (grad->stops[0].pos < 0.5f) ? 1.f : 0.f;
				new_stop.color = (ImVec4){0.25f,0.25f,0.25f,1};
			} else {
				new_stop.pos = (grad->stops[grad->stops_count-2].pos + grad->stops[grad->stops_count-1].pos) * 0.5f;
				new_stop.color = interpolate(stops, grad->stops_count, new_stop.pos);
			}
		} else {
			// From mouse click
			new_stop.pos = command.pos;
			new_stop.color = interpolate(stops, grad->stops_count, new_stop.pos);
		}
		if (is_derived)
			new_stop.is_inserted = true;
		gradient_insert_at(grad, INSERT_END, new_stop);
		changed = true;
	} else if (command.type == COMMAND_REMOVE_AT) {
		uid_t discarded_id = grad->stops[command.idx].id;
		gradient_remove_at(grad, command.idx);
		if (is_derived)
			gradient_mark_discarded(grad, discarded_id);
		changed = true;
	} else if (command.type == COMMAND_REVERT_ALL) {
		gradient_clear_all_overrides(grad);
		changed = true;
	} else if (command.type == COMMAND_REVERT_DISCARDED) {
		gradient_clear_override(grad, command.base_id);
		changed = true;
	} else if (command.type == COMMAND_REVERT_ALL_DISCARDED) {
		gradient_clear_discareded(grad);
		changed = true;
	} else if (command.type == COMMAND_REVERT_AT) {
		uid_t reverted_id = grad->stops[command.idx].id;
		gradient_clear_override(grad, reverted_id);
		changed = true;
	}

	if (changed)
		gradient_sort(grad);

	grad->has_changes |= changed;

	return changed;
}
