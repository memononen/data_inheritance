#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "utils.h"

enum {
	MAX_ACTIONS = 8,
};

typedef enum {
	BUTTON_A,
	BUTTON_B,
	BUTTON_X,
	BUTTON_Y,
} button_code_t;

typedef enum {
	ACTION_FIRE,
	ACTION_MELEE,
	ACTION_JUMP,
	ACTION_DODGE,
	ACTION_DASH,
	ACTION_DUCK,
} action_type_t;

typedef struct action {
	button_code_t button;
	action_type_t action;
	// Meta
	unid_t id;
	bool has_id_conflict;
	bool is_inserted;
	bool override_array_index;
	bool override_button;
	bool override_action;
} action_t;

typedef struct action_map {
	action_t actions[MAX_ACTIONS];
	int32_t actions_count;
	// Meta
	unid_t discarded[MAX_ACTIONS];
	int32_t discarded_count;
	bool has_changes;
} action_map_t;

unid_t actions_add(action_map_t* actions, button_code_t button, action_type_t action);
void actions_validate(action_map_t* actions);

void actions_update_inherited_data(const action_map_t* base_actions, action_map_t* derived_actions);

bool edit_actions(action_map_t* actions, const action_map_t* base_actions);
