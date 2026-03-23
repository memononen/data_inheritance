#include "edit_todos.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "dcimgui.h"
#include "dcimgui_internal.h"
#include "imgui_utils.h"
#include "utils.h"


uid_t todos_add(todo_list_t* todos, const char* name)
{
	if (todos->tasks_count >= MAX_TASKS) return 0;
	int32_t idx = todos->tasks_count++;
	todos->tasks[idx] = (task_t) { .id = gen_id(), };
	strcpy(todos->tasks[idx].name, name);
	return todos->tasks[idx].id;
}

void todos_insert_at(todo_list_t* todos, int32_t idx, task_t action)
{
	if (todos->tasks_count >= MAX_TASKS) return;

	if (idx == -1) idx = todos->tasks_count;

	for (int32_t i = todos->tasks_count; i > idx; i--)
		todos->tasks[i] = todos->tasks[i - 1];
	todos->tasks_count++;

	todos->tasks[idx] = action;
}

void todos_remove_at(todo_list_t* todos, int32_t idx)
{
	todos->tasks_count--;
	for (int32_t i = idx; i < todos->tasks_count; i++)
		todos->tasks[i] = todos->tasks[i + 1];
}

int32_t todos_index_of(const todo_list_t* todos, uid_t id)
{
	for (int32_t i = 0; i < todos->tasks_count; i++)
		if (todos->tasks[i].id == id)
			return i;
	return -1;
}

void todos_remove(todo_list_t* todos, uid_t id)
{
	int32_t idx = todos_index_of(todos, id);
	if (idx == -1)
		return;
	todos_remove_at(todos, idx);
}

void todos_mark_discarded(todo_list_t* todos, uid_t id)
{
	if (id == 0) return;
	for (int32_t i = 0; i < todos->discarded_count; i++) {
		if (todos->discarded[i] == id)
			return;
	}
	if (todos->discarded_count >= MAX_TASKS) return;
	todos->discarded[todos->discarded_count++] = id;
}

void todos_clear_overrides(task_t* action);

void todos_clear_override(todo_list_t* todos, uid_t id)
{
	for (int32_t i = 0; i < todos->tasks_count; i++) {
		if (todos->tasks[i].id == id) {
			if (todos->tasks[i].is_inserted)
				todos_remove_at(todos, i);
			else
				todos_clear_overrides(&todos->tasks[i]);
			return;
		}
	}

	int32_t idx = -1;
	for (int32_t i = 0; i < todos->discarded_count; i++) {
		if (todos->discarded[i] == id) {
			idx = i;
			break;
		}
	}
	if (idx == -1) return;

	todos->discarded_count--;
	for (int32_t i = idx; i < todos->discarded_count; i++)
		todos->discarded[i] = todos->discarded[i + 1];
}

void todos_clear_all_overrides(todo_list_t* todos)
{
	for (int32_t i = 0; i < todos->tasks_count; i++) {
		if (todos->tasks[i].is_inserted) {
			todos_remove_at(todos, i);
			i--;
		} else {
			todos_clear_overrides(&todos->tasks[i]);
		}
	}
	todos->discarded_count = 0;
}

void todos_clear_discarded(todo_list_t* todos)
{
	todos->discarded_count = 0;
}

bool task_has_overrides(const task_t* task);

bool todos_has_overrides(const todo_list_t* todos)
{
	if (todos->discarded_count > 0)
		return true;
	for (int32_t i = 0; i < todos->tasks_count; i++) {
		if (todos->tasks[i].is_inserted)
			return true;
		if (task_has_overrides(&todos->tasks[i]))
			return true;
	}
	return false;
}

int32_t todos_get_discarded_count(const todo_list_t* todos)
{
	return todos->discarded_count;
}


bool task_is_override(const task_t* task)
{
	return task->is_inserted;
}

bool task_has_overrides(const task_t* task)
{
	return task->override_done || task->override_name || task->override_array_index;
}

void todos_clear_overrides(task_t* action)
{
	action->override_done = false;
	action->override_name = false;
	action->override_array_index = false;
}


void todos_update_inherited_data(const todo_list_t* base_todos, todo_list_t* derived_todos)
{
	merge_array_t base = {0};
	merge_array_t derived = {0};

	for (int32_t i = 0; i < base_todos->tasks_count; i++) {
		merge_array_add(&base, (merge_item_t){
			.id = base_todos->tasks[i].id,
			.base_idx = i,
			.derived_idx = INVALID_INDEX,
		});
	}

	for (int32_t i = 0; i < derived_todos->tasks_count; i++) {
		merge_array_add(&derived, (merge_item_t){
			.id = task_is_override(&derived_todos->tasks[i]) ? INVALID_ID : derived_todos->tasks[i].id,
			.base_idx = INVALID_INDEX,
			.derived_idx = i,
			.is_pinned = task_is_override(&derived_todos->tasks[i]) || derived_todos->tasks[i].override_array_index,
		});
	}
	for (int32_t i = 0; i < derived_todos->discarded_count; i++)
		merge_array_add_discarded(&derived, derived_todos->discarded[i]);

	// Do the magic
	merge_array_reconcile(&base, &derived);

	// Copy back adjusted removed items.
	derived_todos->discarded_count = derived.discarded_count;
	for (int32_t i = 0; i < derived.discarded_count; i++)
		derived_todos->discarded[i] = derived.discarded[i];

	// Combine results and inherit properties.
	task_t results[MAX_TASKS];
	int32_t result_tasks_count = 0;

	for (int32_t i = 0; i < derived.items_count; i++) {
		const int32_t base_idx = derived.items[i].base_idx;
		const int32_t derived_idx = derived.items[i].derived_idx;
		if (derived_idx == INVALID_INDEX) {
			// The item does not exist in derived, create new derived item.
			assert(base_idx != INVALID_INDEX);
			const task_t* base_action = &base_todos->tasks[base_idx];
			task_t new_action = {
				.done = base_action->done,
				.id = base_action->id,
			};
			strcpy(new_action.name, base_action->name);
			assert(result_tasks_count < MAX_TASKS);
			results[result_tasks_count++] = new_action;
		} else {
			// Copy item from original array.
			assert(derived_idx != INVALID_INDEX);
			task_t existing_item = derived_todos->tasks[derived_idx];
			// Inherit data
			if (base_idx != INVALID_INDEX && !existing_item.is_inserted) {
				const task_t* base_action = &base_todos->tasks[base_idx];
				if (!existing_item.override_done)
					existing_item.done = base_action->done;
				if (!existing_item.override_name)
					strcpy(existing_item.name, base_action->name);
			}
			assert(result_tasks_count < MAX_TASKS);
			results[result_tasks_count++] = existing_item;
		}
	}

	// Copy results back to derived.
	derived_todos->tasks_count = result_tasks_count;
	for (int32_t i = 0; i < result_tasks_count; i++)
		derived_todos->tasks[i] = results[i];
}


bool edit_todos(todo_list_t* todos, const todo_list_t* base_todos)
{
	const bool is_derived = base_todos != NULL;
	bool changed = false;
	command_t command = {0};
	int32_t discarded_count = todos_get_discarded_count(todos);
	const bool has_overrides = todos_has_overrides(todos);

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

	array_header_state_t header = edit_array_header("To Do", todos->tasks_count, header_flags);

	// Add
	if (header.add_is_clicked) {
		command = command_make_insert_at(todos->tasks_count);
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
				for (int32_t i = 0; i < todos->discarded_count; i++) {
					const int32_t base_idx = todos_index_of(base_todos, todos->discarded[i]);
					const task_t* base_item = base_idx != INVALID_INDEX ? &base_todos->tasks[base_idx] : NULL;
					if (base_item) {
						if (ImGui_MenuItem(base_item->name)) {
							command = command_make_revert_discarded(todos->discarded[i]);
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
			for (int32_t i = 0; i < todos->discarded_count; i++) {
				const int32_t base_idx = todos_index_of(base_todos, todos->discarded[i]);
				const task_t* base_item = base_idx != INVALID_INDEX ? &base_todos->tasks[base_idx] : NULL;
				if (base_item) {
					ImGui_Text("%s", base_item->name);
				}
			}
			ImGui_EndTooltip();
		}
	}

	if (header.is_open) {
		const float row_h = ImGui_GetFrameHeight();

		for (int32_t i = 0; i < todos->tasks_count; i++) {
			task_t* item = &todos->tasks[i];

			ImGui_PushIDInt(i);

			ImGui_PushStyleVarX(ImGuiStyleVar_ItemSpacing, 0);
			ImGui_PushStyleVarImVec2(ImGuiStyleVar_FramePadding, (ImVec2){0,0});
			ImGui_SelectableEx("##row", false, ImGuiSelectableFlags_AllowOverlap, (ImVec2){0, row_h});
			ImRect item_rect = ImGui_GetItemContentRect();
			ImGui_PopStyleVar(); // Frame padding
			ImGui_PopStyleVar(); // Item spacing
			const bool is_row_hovered = ImGui_IsItemHovered(ImGuiHoveredFlags_DelayNone);

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
						if (from != to && from >= 0 && from < todos->tasks_count) {
							command = command_make_reorder_at(from, to);
						}
					}
				}

				ImGui_EndDragDropTarget();
			}

			ImGui_OpenPopupOnItemClick("node_context_menu", 0);

			const bool item_is_overridden = task_is_override(item);
			const bool item_has_overrides = task_has_overrides(item);

			// Override marker for the whole row
			if (is_derived) {
				ImGui_PackNextSlotEx(measure_override_marker(), ImGuiPack_Start, ImGuiAlign_Center, get_override_marker_space());
				override_marker(item_is_overridden, item_has_overrides);

				if (ImGui_BeginItemTooltip()) {
					if (item_is_overridden)
						ImGui_TextUnformatted("Action has been inserted, and does not exists in Base.");
					else if (item_has_overrides)
						ImGui_TextUnformatted("Action has modifications compared to Base.");
					ImGui_EndTooltip();
				}
			}

			// Revert button
			if (is_derived) {
				ImGui_PackNextSlotEx(ImGui_MeasureIconButton(), ImGuiPack_End, ImGuiAlign_Center, 0.f);
				if (ImGui_IconButtonColoredEx(ICON_ARROW_BACK, IM_COL32(255, 255, 255, 128), item_has_overrides || item_is_overridden)) {
					command = command_make_revert_at(i);
				}
				ImGui_SetItemTooltip("Revert changes.");
			}

			// Remove button
			ImGui_PackNextSlotEx(ImGui_MeasureIconButton(), ImGuiPack_End, ImGuiAlign_Center, 0.f);
			if (ImGui_IconButtonColored(ICON_X, IM_COL32(255, 255, 255, 128))) {
				command = command_make_remove_at(i);
			}
			ImGui_SetItemTooltip("Remove action.");

			// Context menu
			if (ImGui_BeginPopup("node_context_menu", 0)) {
				if (ImGui_MenuItem(ICON_PLUS " Add")) {
					command = command_make_insert_at(i+1);
				}
				if (is_derived) {
					ImGui_BeginDisabled(!item->override_array_index);
					if (ImGui_MenuItem(ICON_ARROW_BACK " Revert Reoder")) {
						item->override_array_index = false;
						changed = true;
					}
					ImGui_EndDisabled();
				}

				if (ImGui_MenuItem(ICON_X " Delete")) {
					command = command_make_remove_at(i);
				}
				ImGui_EndPopup();
			}


			// Grab handle
			ImGui_PackNextSlot(ImGui_MeasureIcon(), ImGuiPack_Start, ImGuiAlign_Center);
			ImGui_AlignTextToFramePadding();
			if (item->override_array_index) {
				ImGui_IconColored(ICON_REORDER, IM_COL32(255,255,255,128));
				ImGui_SetItemTooltip("Drag to move. The item has been reodered.");
			} else {
				ImGui_IconColored(ICON_GRIP_HORIZONTAL, IM_COL32(255,255,255,128));
				ImGui_SetItemTooltip("Drag to move.");
			}

			// Done property
			ImGui_PackNextSlot(ImGui_MeasureFrame(1.f), ImGuiPack_Start, ImGuiAlign_Center);
			if (ImGui_Checkbox("##done", &item->done)) {
				if (is_derived && !item->is_inserted)
					item->override_done = true;
				changed = true;
			}
			if (is_derived) {
				if (item_override_marker(item->override_done)) {
					item->override_done = false;
					changed = true;
				}
			}

			// Name property
			ImRect name_rect = ImGui_PackNextSlotPct(1.0f, ImGui_GetFrameHeight(), ImGuiPack_Start, ImGuiAlign_Center);
			ImGui_AlignTextToFramePadding();

			ImGui_TextUnformatted(item->name);
			ImGui_SetItemTooltip("Task name, double click to edit.");

			if (is_row_hovered && ImGui_IsMouseDoubleClicked(0)) {
				ImGui_OpenPopup("rename_popup", 0);
			}

			// Would like to use ImGui_TempInputText() but it assets, popup window to save the day!
			ImGui_PushStyleVarImVec2(ImGuiStyleVar_WindowPadding, (ImVec2){0,0});
			ImGui_SetNextWindowPos(name_rect.Min, ImGuiCond_Appearing);
			if (ImGui_BeginPopup("rename_popup", 0)) {
				ImGui_SetKeyboardFocusHere();
				if (ImGui_InputText("##name", item->name, IM_COUNTOF(item->name), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
					if (is_derived && !item->is_inserted)
						item->override_name = true;
					changed = true;
					ImGui_CloseCurrentPopup();
				}
				ImGui_EndPopup();
			}
			ImGui_PopStyleVar();

			if (is_derived) {
				if (override_marker_box(name_rect, item->override_name)) {
					item->override_name = false;
					changed = true;
				}
			}

			ImGui_EndPack();
			ImGui_PopID();
		}
	}


	if (command.type == COMMAND_INSERT_AT) {
		task_t new_action = { .id = gen_id() };
		strcpy(new_action.name, gen_name("Thing to do"));
		if (is_derived)
			new_action.is_inserted = true;
		todos_insert_at(todos, command.idx, new_action);
		changed = true;
	} else if (command.type == COMMAND_REMOVE_AT) {
		uid_t discarded_id = todos->tasks[command.idx].id;
		todos_remove_at(todos, command.idx);
		if (is_derived)
			todos_mark_discarded(todos, discarded_id);
		changed = true;
	} else if (command.type == COMMAND_REORDER_AT) {
		task_t tmp = todos->tasks[command.idx];
		if (is_derived && !tmp.is_inserted)
			tmp.override_array_index = true;
		todos_remove_at(todos, command.idx);
		todos_insert_at(todos, command.target_idx, tmp);
		changed = true;
	} else if (command.type == COMMAND_REVERT_ALL) {
		todos_clear_all_overrides(todos);
		changed = true;
	} else if (command.type == COMMAND_REVERT_DISCARDED) {
		todos_clear_override(todos, command.base_id);
		changed = true;
	} else if (command.type == COMMAND_REVERT_ALL_DISCARDED) {
		todos_clear_discarded(todos);
		changed = true;
	} else if (command.type == COMMAND_REVERT_AT) {
		uid_t reverted_id = todos->tasks[command.idx].id;
		todos_clear_override(todos, reverted_id);
		changed = true;
	}

	todos->has_changes |= changed;

	return changed;
}
