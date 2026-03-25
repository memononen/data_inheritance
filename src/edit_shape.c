
#include "edit_shape.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "dcimgui.h"
#include "dcimgui_internal.h"
#include "imgui_utils.h"

void shape_update_inherited_data(collision_shape_t* base, collision_shape_t* derived)
{
	if (!derived->override_shape)
		derived->shape = base->shape;
	if (!derived->override_size)
		derived->size = base->size;
	derived->hit_flags = (derived->hit_flags & derived->override_hit_flags) | (base->hit_flags & ~derived->override_hit_flags);
}

bool shape_has_overrides(const collision_shape_t* shape)
{
	return shape->override_shape || shape->override_size || shape->override_hit_flags;
}

typedef struct {
	bool revert_clicked;
	ImRect widget_rect;
} property_row_result_t;

property_row_result_t edit_property_row(const char* property_name, bool is_derived, bool is_overridden)
{
	property_row_result_t result = {0};

	ImGui_BeginPack(ImGui_GetRowRect(ImGui_DefaultRowHeight));

	ImGui_PackNextSlotPct(0.3f, ImGui_GetFrameHeight(), ImGuiPack_Start, ImGuiAlign_Center);
	ImGui_AlignTextToFramePadding();
	ImGui_TextUnformatted(property_name);

	// Restore
	if (is_derived) {
		ImGui_PackNextSlotEx(ImGui_MeasureIconButton(), ImGuiPack_End, ImGuiAlign_Center, 0.f);
		if (ImGui_IconButtonColoredEx(ICON_ARROW_BACK, IM_COL32(255, 255, 255, 128), is_overridden)) {
			result.revert_clicked = true;
		}
		ImGui_SetItemTooltip("Revert changes.");
	}

	result.widget_rect = ImGui_EndPack();

	return result;
}

bool edit_collosion_shape(collision_shape_t* shape, bool is_derived)
{
	bool changed = false;

	uint32_t header_flags = 0;
	if (is_derived) {
		header_flags |= ARRAY_HEADER_HAS_REVERT;

		const bool has_overrides = shape_has_overrides(shape);
		if (has_overrides)
			header_flags |= ARRAY_HEADER_HAS_MODIFIED_ITEMS;
		if (has_overrides)
			header_flags |= ARRAY_HEADER_ALLOW_REVERT;
	}

	// Header
	array_header_state_t header = edit_array_header("Shape", -1, header_flags);

	// Revert
	if (header.revert_is_clicked) {
		shape->override_shape = false;
		shape->override_size = false;
		shape->override_hit_flags = 0;
		changed = true;
	}
	if (header.revert_is_tooltip_hovered) {
		ImGui_SetTooltip("Revert all changes.");
	}

	if (header.is_open) {

		// Shape
		{
			ImGui_PushIDInt(0);
			property_row_result_t res = edit_property_row("Shape", is_derived, shape->override_shape);
			ImGui_SetNextItemRect(res.widget_rect.Min, res.widget_rect.Max);

			if (res.revert_clicked) {
				shape->override_shape = false;
				changed = true;
			}

			static const char* shape_names[] = { "Circle", "Triangle", "Square", };
			int cur_type = shape->shape;
			if (ImGui_ComboChar("##shape", &cur_type, shape_names, IM_COUNTOF(shape_names))) {
				shape->shape = (shape_type_t)cur_type;
				if (is_derived)
					shape->override_shape = true;
				changed = true;
			}
			if (is_derived)
				override_marker_overlay(ImGui_GetItemRect(), shape->override_shape, false);

			ImGui_PopID();
		}

		// Size
		{
			ImGui_PushIDInt(1);
			property_row_result_t res = edit_property_row("Size", is_derived, shape->override_size);

			if (res.revert_clicked) {
				shape->override_shape = false;
				changed = true;
			}

			ImGui_SetNextItemRect(res.widget_rect.Min, res.widget_rect.Max);
			float v = shape->size;
			if (ImGui_InputFloat("##size", &v)) {
				shape->size = v;
				if (is_derived)
					shape->override_size = true;
				changed = true;
			}
			if (is_derived)
				override_marker_overlay(ImGui_GetItemRect(), shape->override_size, false);

			ImGui_PopID();
		}

		// Hit flags
		{
			ImGui_PushIDInt(2);
			property_row_result_t res = edit_property_row("Hit Flags", is_derived, shape->override_hit_flags != 0);
			ImGui_SetNextItemRect(res.widget_rect.Min, res.widget_rect.Max);

			if (res.revert_clicked) {
				shape->override_hit_flags = 0;
				changed = true;
			}

			static const char* hit_flag_names[] = { "Static", "Dynamic", "Sensor", };
			static const int hit_flag_values[] = { HIT_STATIC, HIT_DYNAMIC, HIT_SENSOR, };
			static const int hit_flag_count = IM_COUNTOF(hit_flag_names);

			char preview[256] = "";
			bool first = true;
			for (int32_t i = 0; i < hit_flag_count; i++) {
				if (shape->hit_flags & hit_flag_values[i]) {
					if (!first)
						strcat(preview, ", ");
					strcat(preview, hit_flag_names[i]);
					first = false;
				}
			}

			if (ImGui_BeginCombo("##hit_flags", preview, 0)) {
				for (int32_t i = 0; i < hit_flag_count; i++) {

					const bool is_selected = (shape->hit_flags & hit_flag_values[i]);
					const bool is_override = (shape->override_hit_flags & hit_flag_values[i]);

					ImGui_PushIDInt(i);


					bool item_toggled = ImGui_SelectableEx("##item", false, ImGuiSelectableFlags_AllowOverlap | ImGuiSelectableFlags_NoAutoClosePopups, (ImVec2){0,ImGui_GetFrameHeight()});

					if (is_selected)
						ImGui_SetItemDefaultFocus();

					ImGui_BeginPack(ImGui_GetItemContentRect());

					// Item
					ImGui_PackNextSlot(ImGui_MeasureFrame(1.f), ImGuiPack_Start, ImGuiAlign_Center);
					bool is_selected_dummy = is_selected;
					if (ImGui_Checkbox("##selected", &is_selected_dummy)) // Expecting that the button will not get activated, since the selectable does not allow overlap.
						item_toggled = true;

					if (is_derived)
						override_marker_overlay(ImGui_GetItemRect(),  is_override, false);

					ImGui_PackNextSlot(ImGui_MeasureTextUnformatted(hit_flag_names[i]), ImGuiPack_Start, ImGuiAlign_Center);
					ImGui_AlignTextToFramePadding();
					ImGui_TextUnformatted(hit_flag_names[i]);

					// Revert
					if (is_derived) {
						ImGui_PackNextSlot(ImGui_MeasureIconButton(), ImGuiPack_End, ImGuiAlign_Center);
						if (ImGui_IconButtonEx(ICON_ARROW_BACK, is_override)) {
							// Clear override
							shape->override_hit_flags &= ~hit_flag_values[i];
							changed = true;
						}
					}

					ImGui_EndPack();
					ImGui_PopID();

					if (item_toggled) {
						shape->hit_flags ^= hit_flag_values[i];
						shape->override_hit_flags |= hit_flag_values[i];
						changed = true;
					}
				}
				ImGui_EndCombo();
			}
			if (is_derived)
				override_marker_overlay(ImGui_GetItemRect(),  shape->override_hit_flags, false);


			ImGui_PopID();

		}
	}

	shape->has_changes |= changed;

	return changed;
}
