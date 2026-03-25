#include "edit_nodes.h"

#include <string.h>

#include "dcimgui.h"
#include "dcimgui_internal.h"
#include "imgui_utils.h"
#include "utils.h"

int32_t nodes_add(node_ref_array_t* nodes, const char* name)
{
	if (nodes->nodes_count >= MAX_NODES) return 0;
	int32_t idx = nodes->nodes_count++;
	node_ref_t* item = &nodes->nodes[idx];
	memset(item, 0, sizeof(node_ref_t));
	strncpy(item->name, name, sizeof(item->name));
	item->is_visible = true;
	item->id = gen_id();
	return item->id;
}

void nodes_insert_at(node_ref_array_t* nodes, int32_t idx, node_ref_t node)
{
	if (nodes->nodes_count >= MAX_NODES) return;

	if (idx == -1) idx = nodes->nodes_count;
	assert(idx >= 0 && idx <= nodes->nodes_count);

	for (int32_t i = nodes->nodes_count; i > idx; i--)
		nodes->nodes[i] = nodes->nodes[i - 1];
	nodes->nodes_count++;

	node_ref_t* item = &nodes->nodes[idx];
	memset(item, 0, sizeof(node_ref_t));
	*item = node;
}

void nodes_remove_at(node_ref_array_t* nodes, int32_t idx)
{
	nodes->nodes_count--;
	for (int32_t i = idx; i < nodes->nodes_count; i++)
		nodes->nodes[i] = nodes->nodes[i + 1];
}

int32_t nodes_index_of(const node_ref_array_t* nodes, uid_t id)
{
	if (id == INVALID_ID) return INVALID_INDEX;
	for (int32_t i = 0; i < nodes->nodes_count; i++)
		if (nodes->nodes[i].id == id)
			return i;
	return INVALID_INDEX;
}

void nodes_remove(node_ref_array_t* nodes, uid_t id)
{
	int32_t idx = nodes_index_of(nodes, id);
	if (idx == INVALID_INDEX) return;
	nodes_remove_at(nodes, idx);
}


bool nodes_is_override(const node_ref_array_t* nodes, uid_t id)
{
	for (int32_t i = 0; i < nodes->nodes_count; i++)
		if (nodes->nodes[i].id == id)
			return nodes->nodes[i].base_id == 0;
	for (int32_t i = 0; i < nodes->discarded_count; i++)
		if (nodes->discarded[i] == id)
			return true;
	return false;
}

void nodes_mark_discarded(node_ref_array_t* nodes, uid_t id)
{
	for (int32_t i = 0; i < nodes->discarded_count; i++)
		if (nodes->discarded[i] == id)
			return;
	if (nodes->discarded_count >= MAX_NODES) return;
	nodes->discarded[nodes->discarded_count++] = id;
}

void nodes_clear_override(node_ref_array_t* nodes, uid_t id)
{
	for (int32_t i = 0; i < nodes->nodes_count; i++) {
		if (nodes->nodes[i].id == id) {
			nodes_remove_at(nodes, i);
			return;
		}
	}

	int32_t idx = INVALID_INDEX;
	for (int32_t i = 0; i < nodes->discarded_count; i++) {
		if (nodes->discarded[i] == id) {
			idx = i;
			break;
		}
	}
	if (idx == INVALID_INDEX) return;
	nodes->discarded_count--;
	for (int32_t i = idx; i < nodes->discarded_count; i++)
		nodes->discarded[i] = nodes->discarded[i + 1];
}

void node_clear_overrides(node_ref_t* node);
void nodes_clear_all_overrides(node_ref_array_t* nodes)
{
	for (int32_t i = 0; i < nodes->nodes_count; i++) {
		if (nodes->nodes[i].is_inserted) {
			nodes_remove_at(nodes, i);
			i--;
		} else {
			node_clear_overrides(&nodes->nodes[i]);
		}
	}

	nodes->discarded_count = 0;
}

void nodes_clear_discared(node_ref_array_t* nodes)
{
	nodes->discarded_count = 0;
}

bool node_has_overrides(const node_ref_t* node);
bool nodes_has_overrides(const node_ref_array_t* nodes)
{
	// Has removes
	if (nodes->discarded_count > 0)
		return true;
	for (int32_t i = 0; i < nodes->nodes_count; i++) {
		// Has reordered
		if (node_has_overrides(&nodes->nodes[i]))
			return true;
		// Has inserted
		if (nodes->nodes[i].is_inserted)
			return true;
	}
	return false;
}

int32_t nodes_get_discarded_count(const node_ref_array_t* nodes)
{
	return nodes->discarded_count;
}


bool node_is_override(const node_ref_t* node)
{
	return node->is_inserted;
}

bool node_has_overrides(const node_ref_t* node)
{
	return node->override_array_index || node->override_is_visible;
}

void node_clear_overrides(node_ref_t* node)
{
	node->override_array_index = false;
	node->override_is_visible = false;
}


void nodes_update_inherited_data(const node_ref_array_t* base_nodes, node_ref_array_t* derived_nodes)
{
	merge_array_t base = {0};
	merge_array_t derived = {0};

	for (int32_t i = 0; i < base_nodes->nodes_count; i++) {
		merge_array_add(&base, (merge_item_t){
			.id = base_nodes->nodes[i].id,
			.base_idx = i,
			.derived_idx = INVALID_INDEX,
		});
	}

	for (int32_t i = 0; i < derived_nodes->nodes_count; i++) {
		merge_array_add(&derived, (merge_item_t){
			.id = node_is_override(&derived_nodes->nodes[i]) ? INVALID_ID : derived_nodes->nodes[i].base_id,
			.base_idx = INVALID_INDEX,
			.derived_idx = i,
			.is_pinned = node_is_override(&derived_nodes->nodes[i]) || derived_nodes->nodes[i].override_array_index,
		});
	}
	for (int32_t i = 0; i < derived_nodes->discarded_count; i++)
		merge_array_add_discarded(&derived, derived_nodes->discarded[i]);

	merge_array_reconcile(&base, &derived);

	// Copy adjusted removed items.
	derived_nodes->discarded_count = derived.discarded_count;
	for (int32_t i = 0; i < derived.discarded_count; i++)
		derived_nodes->discarded[i] = derived.discarded[i];

	// Combine results and inherit properties.
	node_ref_t result_nodes[MAX_NODES];
	int32_t result_nodes_count = 0;

	for (int32_t i = 0; i < derived.items_count; i++) {
		const int32_t base_idx = derived.items[i].base_idx;
		const int32_t derived_idx = derived.items[i].derived_idx;
		if (derived_idx == INVALID_INDEX) {
			// The item does not exist in derived, create new derived node.
			assert(base_idx != INVALID_INDEX);
			const node_ref_t* base_node = &base_nodes->nodes[base_idx];

			node_ref_t new_node = {
				.id = gen_id(),
				.base_id = base_node->id,
				.is_visible = base_node->is_visible,
			};
			memcpy(new_node.name, base_node->name, sizeof(new_node.name));

			assert(result_nodes_count < MAX_NODES);
			result_nodes[result_nodes_count++] = new_node;
		} else {
			// Copy node from original array.
			assert(derived_idx != INVALID_INDEX);
			node_ref_t existing_node = derived_nodes->nodes[derived_idx];
			// Inherit data
			const int32_t base_node_idx = nodes_index_of(base_nodes, existing_node.base_id);
			if (base_node_idx != INVALID_INDEX) {
				const node_ref_t* base_node = &base_nodes->nodes[base_node_idx];
				if (!existing_node.override_is_visible)
					existing_node.is_visible = base_node->is_visible;
			} else {
				existing_node.base_id = INVALID_ID;
			}

			assert(result_nodes_count < MAX_NODES);
			result_nodes[result_nodes_count++] = existing_node;
		}
	}

	// Copy results back to derived.
	derived_nodes->nodes_count = result_nodes_count;
	for (int32_t i = 0; i < result_nodes_count; i++)
		derived_nodes->nodes[i] = result_nodes[i];
}

static bool edit_base_node_menu(const node_ref_array_t* base_nodes, uid_t* base_node_id)
{
	assert(base_nodes);
	bool result = false;

	for (int32_t i = 0; i < base_nodes->nodes_count; i++) {
		const node_ref_t* item = &base_nodes->nodes[i];
		ImGui_PushIDInt(i);
		if (ImGui_MenuItemWithIcon(item->name, ICON_BOX)) {
			*base_node_id = item->id;
			result = true;
		}
		ImGui_PopID();
	}
	return result;
}

bool edit_nodes(node_ref_array_t* nodes, const node_ref_array_t* base_nodes)
{
	bool changed = false;
	command_t command = {0};
	int32_t insert_index = INSERT_END;
	bool is_derived = base_nodes != NULL;
	const int32_t discarded_count = nodes_get_discarded_count(nodes);
	const bool has_overrides = nodes_has_overrides(nodes);

	uint32_t header_flags = ARRAY_HEADER_ALLOW_ADD;
	if (is_derived) {
		header_flags |= ARRAY_HEADER_ALLOW_INHERIT;
		header_flags |= ARRAY_HEADER_HAS_REVERT;
		if (has_overrides)
			header_flags |= ARRAY_HEADER_HAS_MODIFIED_ITEMS;
		if (has_overrides)
			header_flags |= ARRAY_HEADER_ALLOW_REVERT;
		if (discarded_count > 0)
			header_flags |= ARRAY_HEADER_HAS_REMOVED_ITEMS;
	}

	array_header_state_t header = edit_array_header("Nodes", nodes->nodes_count, header_flags);

	// Add
	if (header.add_is_clicked) {
		insert_index = nodes->nodes_count;
		command = command_make_insert_at(nodes->nodes_count);
	}
	if (header.add_is_tooltip_hovered)
		ImGui_SetTooltip("Add new node.");

	// Inherit
	if (header.inherit_is_clicked) {
		insert_index = nodes->nodes_count;
		ImGui_OpenPopup("add_inherited_menu", 0);
		ImGui_SetNextWindowPos(header.menu_pos, ImGuiCond_Always);
	}
	if (header.inherit_is_tooltip_hovered) {
		ImGui_SetTooltip("Add new inherited Node.");
	}
	if (ImGui_BeginPopup("add_inherited_menu", 0)) {
		if (base_nodes) {
			int32_t base_node_id = INVALID_ID;
			if (edit_base_node_menu(base_nodes, &base_node_id)) {
				command = command_make_insert_at_base_id(insert_index, base_node_id);
			}
		}
		ImGui_EndPopup();
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
			command = command_make_revert_all();
		}

		ImGui_BeginDisabled(discarded_count == 0);
		if (ImGui_MenuItemWithIcon("Revert All Removed", "")) {
			command = command_make_revert_all_discarded();
		}
		ImGui_EndDisabled();

		if (discarded_count > 0) {
			if (ImGui_BeginMenu("Revert Removed")) {
				for (int32_t i = 0; i < nodes->discarded_count; i++) {
					const int32_t base_idx = nodes_index_of(base_nodes, nodes->discarded[i]);
					const node_ref_t* base_item = base_idx != INVALID_INDEX ? &base_nodes->nodes[base_idx] : NULL;
					if (base_item) {
						if (ImGui_MenuItemWithIcon(base_item->name, ICON_BOX)) {
							command = command_make_revert_discarded(nodes->discarded[i]);
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
			ImGui_Text("%d items removed.", nodes->discarded_count);
			for (int32_t i = 0; i < nodes->discarded_count; i++) {
				const int32_t base_idx = nodes_index_of(base_nodes, nodes->discarded[i]);
				const node_ref_t* base_item = base_idx != INVALID_INDEX ? &base_nodes->nodes[base_idx] : NULL;
				if (base_item) {
					ImGui_TextUnformatted(ICON_BOX);
					ImGui_SameLine();
					ImGui_TextUnformatted(base_item->name);
				}
			}
			ImGui_EndTooltip();
		}
	}

	const float row_h = ImGui_GetFrameHeight();

	if (header.is_open) {

		for (int32_t i = 0; i < nodes->nodes_count; i++) {
			node_ref_t* item = &nodes->nodes[i];

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
				ImGui_SetDragDropPayload("DND_ITEM", &i, sizeof(int32_t), ImGuiCond_Once);

				ImGui_Text("Move");
				ImGui_EndDragDropSource();
			}

			// Drag & Drop
			if (ImGui_BeginDragDropTarget()) {
				const ImGuiPayload* payload = ImGui_AcceptDragDropPayload("DND_ITEM", ImGuiDragDropFlags_AcceptNoDrawDefaultRect | ImGuiDragDropFlags_AcceptBeforeDelivery);
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
						if (from != to && from >= 0 && from < nodes->nodes_count) {
							command = command_make_reorder_at(from, to);
						}
					}
				}

				ImGui_EndDragDropTarget();
			}

			ImGui_OpenPopupOnItemClick("node_context_menu", 0);

			const bool item_is_override = node_is_override(item);
			const bool item_has_overrides = node_has_overrides(item);

			// Override marker for the whole row
			if (is_derived)
				override_marker_overlay(item_rect, item_is_override, item_has_overrides);

			// Icon
			ImGui_PackNextSlot(ImGui_MeasureIcon(), ImGuiPack_Start, ImGuiAlign_Center);
			ImGui_IconColored(ICON_BOX, IM_COL32(255, 255, 255, 128));

			// Text
			ImGui_PackNextSlot(ImGui_MeasureTextUnformatted(item->name), ImGuiPack_Start, ImGuiAlign_Center);
			ImGui_AlignTextToFramePadding();
			ImGui_TextUnformatted(item->name);

			// Inheritance
			if (item_is_override && item->base_id && base_nodes) {
				const int32_t base_idx = nodes_index_of(base_nodes, item->base_id);
				if (base_idx != -1) {
					const node_ref_t* base_item = &base_nodes->nodes[base_idx];
					ImGui_SameLine();
					ImGui_AlignTextToFramePadding();
					ImGui_TextColored(ImGui_GetColorVec4U32(IM_COL32(255, 255, 255, 64)), "(%s)", base_item->name);
				}
			}


/*			// Debug
			if (item->base_id != 0) {
				ImGui_SameLine();
				ImGui_AlignTextToFramePadding();
				ImGui_TextColored(ImGui_GetColorVec4U32(IM_COL32(255, 255, 255, 128)), "(ID:%s  Base:%s)", id_to_str(item->id), id_to_str(item->base_id));
			} else {
				ImGui_SameLine();
				ImGui_AlignTextToFramePadding();
				ImGui_TextColored(ImGui_GetColorVec4U32(IM_COL32(255, 255, 255, 128)),"(ID:%s)", id_to_str(item->id));
			}*/

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
					if (ImGui_MenuItemWithIconEx("Revert property Visible", ICON_EDIT, NULL, false, item->override_is_visible)) {
						item->override_is_visible = false;
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
			ImGui_SetItemTooltip("Remove node.");

			// Visibility
			ImGui_PackNextSlotEx(ImGui_MeasureIconButton(), ImGuiPack_End, ImGuiAlign_Center, get_override_marker_space());
			char str[32] = "";
			strcat(str, item->is_visible ? ICON_EYE : ICON_EYE_CLOSED);
			strcat(str, "##vis");
			if (ImGui_IconButtonColored(str, IM_COL32(255, 255, 255, 128))) {
				item->is_visible = !item->is_visible;
				if (is_derived && item->base_id != 0)
					item->override_is_visible = true;
				changed = true;
			}
			if (is_derived)
				override_marker_overlay(ImGui_GetItemRect(),  item->override_is_visible, false);

			// Context menu
			if (ImGui_BeginPopup("node_context_menu", 0)) {
				if (ImGui_MenuItemWithIcon("Add Node", ICON_PLUS)) {
					command = command_make_insert_at(i+1);
				}
				if (base_nodes) {
					if (ImGui_BeginMenuWithIcon("Add Inherited", ICON_COPY_PLUS)) {
						int32_t base_node_id = INVALID_ID;
						if (edit_base_node_menu(base_nodes, &base_node_id)) {
							command = command_make_insert_at_base_id(i+1, base_node_id);
						}
						ImGui_EndMenu();
					}
					if (ImGui_MenuItemWithIconEx("Break Inheritance", ICON_COPY_OFF, NULL, false, item->is_inserted && item->base_id != INVALID_ID)) {
						command = command_make_break_inheritance_at(i);
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
		node_ref_t new_node = { .id = gen_id(), .is_visible = true };
		if (is_derived) {
			new_node.is_inserted = true;
			new_node.base_id = command.base_id;
		}
		strncpy(new_node.name, gen_name("Item"), sizeof(new_node.name));
		nodes_insert_at(nodes, command.idx, new_node);
		changed = true;
	} else if (command.type == COMMAND_REMOVE_AT) {
		uid_t discarded_id = nodes->nodes[command.idx].base_id;
		nodes_remove_at(nodes, command.idx);
		if (is_derived)
			nodes_mark_discarded(nodes, discarded_id);
		changed = true;
	} else if (command.type == COMMAND_REORDER_AT) {
		node_ref_t tmp = nodes->nodes[command.idx];
		if (is_derived && !node_is_override(&tmp))
			tmp.override_array_index = true;
		nodes_remove_at(nodes, command.idx);
		nodes_insert_at(nodes, command.target_idx, tmp);
		changed = true;
	} else if (command.type == COMMAND_REVERT_ALL) {
		nodes_clear_all_overrides(nodes);
		changed = true;
	} else if (command.type == COMMAND_REVERT_DISCARDED) {
		nodes_clear_override(nodes, command.base_id);
		changed = true;
	} else if (command.type == COMMAND_REVERT_ALL_DISCARDED) {
		nodes_clear_discared(nodes);
		changed = true;
	} else if (command.type == COMMAND_REVERT_AT) {
		uid_t reverted_id = nodes->nodes[command.idx].id;
		nodes_clear_override(nodes, reverted_id);
		changed = true;
	} else if (command.type == COMMAND_BREAK_INHERITANCE_AT) {
		nodes->nodes[command.idx].base_id = INVALID_ID;
		changed = true;
	}

	nodes->has_changes |= changed;

	return changed;
}
