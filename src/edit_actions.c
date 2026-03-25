#include "edit_actions.h"

#include <stdbool.h>
#include <stdint.h>

#include "dcimgui.h"
#include "dcimgui_internal.h"
#include "imgui_utils.h"
#include "utils.h"

uid_t actions_add(action_map_t* actions, button_code_t button, action_type_t action)
{
	if (actions->actions_count >= MAX_ACTIONS) return 0;
	int32_t idx = actions->actions_count++;
	actions->actions[idx] = (action_t) {
		.button = button,
		.action = action,
		.id = gen_id(),
	};
	return actions->actions[idx].id;
}

void actions_insert_at(action_map_t* actions, int32_t idx, action_t action)
{
	if (actions->actions_count >= MAX_ACTIONS) return;

	if (idx == -1) idx = actions->actions_count;

	for (int32_t i = actions->actions_count; i > idx; i--)
		actions->actions[i] = actions->actions[i - 1];
	actions->actions_count++;

	actions->actions[idx] = action;
}

void actions_remove_at(action_map_t* actions, int32_t idx)
{
	actions->actions_count--;
	for (int32_t i = idx; i < actions->actions_count; i++)
		actions->actions[i] = actions->actions[i + 1];
}

int32_t actions_index_of(const action_map_t* actions, uid_t id)
{
	for (int32_t i = 0; i < actions->actions_count; i++)
		if (actions->actions[i].id == id)
			return i;
	return -1;
}

void actions_remove(action_map_t* actions, uid_t id)
{
	int32_t idx = actions_index_of(actions, id);
	if (idx == -1)
		return;
	actions_remove_at(actions, idx);
}

void actions_mark_discarded(action_map_t* actions, uid_t id)
{
	if (id == 0) return;
	for (int32_t i = 0; i < actions->discarded_count; i++) {
		if (actions->discarded[i] == id)
			return;
	}
	if (actions->discarded_count >= MAX_ACTIONS) return;
	actions->discarded[actions->discarded_count++] = id;
}

void action_clear_overrides(action_t* action);

void actions_clear_override(action_map_t* actions, uid_t id)
{
	for (int32_t i = 0; i < actions->actions_count; i++) {
		if (actions->actions[i].id == id) {
			if (actions->actions[i].is_inserted)
				actions_remove_at(actions, i);
			else
				action_clear_overrides(&actions->actions[i]);
			return;
		}
	}

	int32_t idx = -1;
	for (int32_t i = 0; i < actions->discarded_count; i++) {
		if (actions->discarded[i] == id) {
			idx = i;
			break;
		}
	}
	if (idx == -1) return;

	actions->discarded_count--;
	for (int32_t i = idx; i < actions->discarded_count; i++)
		actions->discarded[i] = actions->discarded[i + 1];
}

void actions_clear_all_overrides(action_map_t* actions)
{
	for (int32_t i = 0; i < actions->actions_count; i++) {
		if (actions->actions[i].is_inserted) {
			actions_remove_at(actions, i);
			i--;
		} else {
			action_clear_overrides(&actions->actions[i]);
		}
	}
	actions->discarded_count = 0;
}

void actions_clear_discarded(action_map_t* actions)
{
	actions->discarded_count = 0;
}

bool action_has_overrides(const action_t* action);

bool actions_has_overrides(const action_map_t* actions)
{
	if (actions->discarded_count > 0)
		return true;
	for (int32_t i = 0; i < actions->actions_count; i++) {
		if (actions->actions[i].is_inserted)
			return true;
		if (action_has_overrides(&actions->actions[i]))
			return true;
	}
	return false;
}

int32_t actions_get_discarded_count(const action_map_t* actions)
{
	return actions->discarded_count;
}


bool action_is_override(const action_t* action)
{
	return action->is_inserted;
}

bool action_has_overrides(const action_t* action)
{
	return action->override_button || action->override_action || action->override_array_index;
}

void action_clear_overrides(action_t* action)
{
	action->override_button = false;
	action->override_action = false;
	action->override_array_index = false;
}

void actions_validate(action_map_t* actions)
{
	for (int32_t i = 0; i < actions->actions_count; i++) {
		action_t* item = &actions->actions[i];
		// Check for conflicts
		item->has_id_conflict = false;
		for (int32_t j = 0; j < actions->actions_count; j++) {
			// Skip self
			if (i == j) continue;
			if (item->button == actions->actions[j].button) {
				item->has_id_conflict = true;
				break;
			}
		}
	}
}


void actions_update_inherited_data(const action_map_t* base_actions, action_map_t* derived_actions)
{
	merge_array_t base = {0};
	merge_array_t derived = {0};

	for (int32_t i = 0; i < base_actions->actions_count; i++) {
		if (base_actions->actions[i].id == INVALID_ID)
			continue;
		merge_array_add(&base, (merge_item_t){
			.id = base_actions->actions[i].id,
			.base_idx = i,
			.derived_idx = INVALID_INDEX,
		});
	}

	for (int32_t i = 0; i < derived_actions->actions_count; i++) {
		merge_array_add(&derived, (merge_item_t){
			.id = action_is_override(&derived_actions->actions[i]) ? INVALID_ID : derived_actions->actions[i].id,
			.base_idx = INVALID_INDEX,
			.derived_idx = i,
			.is_pinned = action_is_override(&derived_actions->actions[i])
				|| derived_actions->actions[i].override_array_index,
		});
	}
	for (int32_t i = 0; i < derived_actions->discarded_count; i++)
		merge_array_add_discarded(&derived, derived_actions->discarded[i]);

	// Do the magic
	merge_array_reconcile(&base, &derived);

	// Copy back adjusted removed items.
	derived_actions->discarded_count = derived.discarded_count;
	for (int32_t i = 0; i < derived.discarded_count; i++)
		derived_actions->discarded[i] = derived.discarded[i];

	// Combine results and inherit properties.
	action_t result_actions[MAX_ACTIONS];
	int32_t result_actions_count = 0;

	for (int32_t i = 0; i < derived.items_count; i++) {
		const int32_t base_idx = derived.items[i].base_idx;
		const int32_t derived_idx = derived.items[i].derived_idx;
		if (derived_idx == INVALID_INDEX) {
			// The item does not exist in derived, create new derived item.
			assert(base_idx != INVALID_INDEX);
			const action_t* base_action = &base_actions->actions[base_idx];
			action_t new_action = {
				.button = base_action->button,
				.action = base_action->action,
				.id = base_action->id,
			};
			assert(result_actions_count < MAX_ACTIONS);
			result_actions[result_actions_count++] = new_action;
		} else {
			// Copy item from original array.
			assert(derived_idx != INVALID_INDEX);
			action_t existing_action = derived_actions->actions[derived_idx];
			// Inherit data
			if (base_idx != INVALID_INDEX && !existing_action.is_inserted) {
				assert(derived_idx != INVALID_INDEX);
				const action_t* base_action = &base_actions->actions[base_idx];
				if (!existing_action.override_button)
					existing_action.button = base_action->button;
				if (!existing_action.override_action)
					existing_action.action = base_action->action;
			}
			assert(result_actions_count < MAX_ACTIONS);
			result_actions[result_actions_count++] = existing_action;
		}
	}

	// Copy results back to derived.
	derived_actions->actions_count = result_actions_count;
	for (int32_t i = 0; i < result_actions_count; i++)
		derived_actions->actions[i] = result_actions[i];

	actions_validate(derived_actions);
}


bool edit_actions(action_map_t* actions, const action_map_t* base_actions)
{
	static const char* button_names[] = { "A", "B", "X", "Y", };
	static const char* action_names[] = { "Fire", "Melee", "Jump", "Dodge", "Dash", "Duck", };

	const bool is_derived = base_actions != NULL;
	bool changed = false;
	command_t command = {0};
	int32_t discarded_count = actions_get_discarded_count(actions);
	const bool has_overrides = actions_has_overrides(actions);

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

	array_header_state_t header = edit_array_header("Actions", actions->actions_count, header_flags);

	// Add
	if (header.add_is_clicked) {
		command = command_make_insert_at(actions->actions_count);
	}
	if (header.add_is_tooltip_hovered)
		ImGui_SetTooltip("Add new action.");

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
				for (int32_t i = 0; i < actions->discarded_count; i++) {
					const int32_t base_idx = actions_index_of(base_actions, actions->discarded[i]);
					const action_t* base_item = base_idx != INVALID_INDEX ? &base_actions->actions[base_idx] : NULL;
					if (base_item) {
						char label[64];
						snprintf(label, sizeof(label), "%s > %s.", button_names[base_item->button], action_names[base_item->action]);
						if (ImGui_MenuItem(label)) {
							command = command_make_revert_discarded(actions->discarded[i]);
						}
					}
				}
				ImGui_EndMenu();
			}
		}

		ImGui_EndPopup();
	}


	// Revert discarded
	if (header.removed_is_tooltip_hovered) {
		if (ImGui_BeginTooltip()) {
			ImGui_Text("%d items removed.", discarded_count);
			for (int32_t i = 0; i < actions->discarded_count; i++) {
				const int32_t base_idx = actions_index_of(base_actions, actions->discarded[i]);
				const action_t* base_item = base_idx != INVALID_INDEX ? &base_actions->actions[base_idx] : NULL;
				if (base_item) {
					ImGui_Text("%s > %s.", button_names[base_item->button], action_names[base_item->action]);
				}
			}
			ImGui_EndTooltip();
		}
	}

	if (header.is_open) {
		const float row_h = ImGui_GetFrameHeight();

		for (int32_t i = 0; i < actions->actions_count; i++) {
			action_t* item = &actions->actions[i];

			ImGui_PushIDInt(i);

			ImGui_PushStyleVarImVec2(ImGuiStyleVar_ItemSpacing, (ImVec2){0,0});
			ImGui_PushStyleVarImVec2(ImGuiStyleVar_FramePadding, (ImVec2){0,0});
			ImGui_SelectableEx("", false, ImGuiSelectableFlags_AllowOverlap, (ImVec2){0, row_h});
			ImRect item_rect = ImGui_GetItemContentRect();
			ImGui_PopStyleVar(); // Frame padding
			ImGui_PopStyleVar(); // Item spacing

			ImGui_BeginPack(item_rect);

			if (ImGui_BeginDragDropSource(ImGuiDragDropFlags_None)) {
				// Set payload to carry the index of our item (could be anything)
				ImGui_SetDragDropPayload("DND_ACTION_ITEM", &i, sizeof(int32_t), ImGuiCond_Once);

				ImGui_Text("Move");
				ImGui_EndDragDropSource();
			}

			// Drag & Drop
			if (ImGui_BeginDragDropTarget()) {
				const ImGuiPayload* payload = ImGui_AcceptDragDropPayload("DND_ACTION_ITEM", ImGuiDragDropFlags_AcceptNoDrawDefaultRect | ImGuiDragDropFlags_AcceptBeforeDelivery);
				if (payload) {
					IM_ASSERT(payload->DataSize == sizeof(int32_t));
					const int32_t from = *(const int32_t*)payload->Data;
					const int32_t to = i;

					ImRect drop_rect = item_rect;
					if (to < from) {
						drop_rect.Max.y = drop_rect.Min.y;
						ImGui_RenderDragDropTargetRectForItem(drop_rect);
					} else if (to > from) {
						drop_rect.Min.y = drop_rect.Max.y;
						ImGui_RenderDragDropTargetRectForItem(drop_rect);
					}

					if (ImGuiPayload_IsDelivery(payload)) {
						if (from != to && from >= 0 && from < actions->actions_count) {
							command = command_make_reorder_at(from, to);
						}
					}
				}

				ImGui_EndDragDropTarget();
			}

			ImGui_OpenPopupOnItemClick("node_context_menu", 0);

			const bool item_is_override = action_is_override(item);
			const bool item_has_overrides = action_has_overrides(item);

			// Override marker for the whole row
			if (is_derived)
				override_marker_overlay(item_rect, item_is_override, item_has_overrides);

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
					if (ImGui_MenuItemWithIconEx("Revert reorder", ICON_REORDER, NULL, false, item->override_array_index)) {
						item->override_array_index = false;
						changed = true;
					}
					if (ImGui_MenuItemWithIconEx("Revert property Button", ICON_EDIT, NULL, false, item->override_button)) {
						item->override_button = false;
						changed = true;
					}
					if (ImGui_MenuItemWithIconEx("Revert property Action", ICON_EDIT, NULL, false, item->override_action)) {
						item->override_action = false;
						changed = true;
					}
					ImGui_EndPopup();
				}
			}

			// Remove button
			ImGui_PackNextSlotEx(ImGui_MeasureIconButton(), ImGuiPack_End, ImGuiAlign_Center, 0.f);
			if (ImGui_IconButtonColored(ICON_X, IM_COL32(255, 255, 255, 128))) {
				command = command_make_remove_at(i);
			}
			ImGui_SetItemTooltip("Remove action.");

			// Grab handle
			ImGui_PackNextSlot(ImGui_MeasureIcon(), ImGuiPack_Start, ImGuiAlign_Center);
			ImGui_AlignTextToFramePadding();
			if (item->has_id_conflict) {
				ImGui_IconColored(ICON_ALERT_TRIANGLE_FILLED, IM_COL32(255,160,0,220));
				ImGui_SetItemTooltip("Item key conflicts with another item.");
			} else {
				ImGui_IconColored(ICON_GRIP_HORIZONTAL, IM_COL32(255,255,255,128));
				ImGui_SetItemTooltip("Drag to move.");
			}

			// Button property
			ImGui_PackNextSlot(ImGui_MeasureFrame(5.f), ImGuiPack_Start, ImGuiAlign_Center);
			int cur_button = item->button;
			if (ImGui_ComboChar("##button", &cur_button, button_names, IM_COUNTOF(button_names))) {
				item->button = (button_code_t)cur_button;
				if (is_derived && !item->is_inserted)
					item->override_button = true;

				changed = true;
			}
			if (is_derived)
				override_marker_overlay(ImGui_GetItemRect(), item->override_button, false);

			// Action property
			ImGui_PackNextSlotPct(1.0f, ImGui_GetFrameHeight(), ImGuiPack_Start, ImGuiAlign_Center);
			int cur_action = item->action;
			if (ImGui_ComboChar("##action", &cur_action, action_names, IM_COUNTOF(action_names))) {
				item->action = (action_type_t)cur_action;
				if (is_derived && !item->is_inserted)
					item->override_action = true;
				changed = true;
			}
			if (is_derived)
				override_marker_overlay(ImGui_GetItemRect(), item->override_action, false);

			// Context menu
			if (ImGui_BeginPopup("node_context_menu", 0)) {
				if (ImGui_MenuItemWithIcon("Add new action", ICON_PLUS)) {
					command = command_make_insert_at(i+1);
				}
				if (is_derived) {
					if (ImGui_MenuItemWithIconEx("Revert Reoder", ICON_ARROW_BACK, NULL, false, item->override_array_index)) {
						item->override_array_index = false;
						changed = true;
					}
				}
				if (ImGui_MenuItemWithIcon("Delete", ICON_X)) {
					command = command_make_remove_at(i);
				}
				ImGui_EndPopup();
			}

			ImGui_EndPack();
			ImGui_PopID();
		}
	}


	if (command.type == COMMAND_INSERT_AT) {
		action_t new_action = { .id = gen_id(), };
		if (is_derived)
			new_action.is_inserted = true;
		actions_insert_at(actions, command.idx, new_action);
		changed = true;
	} else if (command.type == COMMAND_REMOVE_AT) {
		uid_t discarded_id = actions->actions[command.idx].id;
		actions_remove_at(actions, command.idx);
		if (is_derived)
			actions_mark_discarded(actions, discarded_id);
		changed = true;
	} else if (command.type == COMMAND_REORDER_AT) {
		action_t tmp = actions->actions[command.idx];
		if (is_derived && !tmp.is_inserted)
			tmp.override_array_index = true;
		actions_remove_at(actions, command.idx);
		actions_insert_at(actions, command.target_idx, tmp);
		changed = true;
	} else if (command.type == COMMAND_REVERT_ALL) {
		actions_clear_all_overrides(actions);
		changed = true;
	} else if (command.type == COMMAND_REVERT_DISCARDED) {
		actions_clear_override(actions, command.base_id);
		changed = true;
	} else if (command.type == COMMAND_REVERT_ALL_DISCARDED) {
		actions_clear_discarded(actions);
		changed = true;
	} else if (command.type == COMMAND_REVERT_AT) {
		uid_t reverted_id = actions->actions[command.idx].id;
		actions_clear_override(actions, reverted_id);
		changed = true;
	}

	if (changed) {
		// Check for key conflicts
		actions_validate(actions);
	}

	actions->has_changes |= changed;

	return changed;
}
