#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "utils.h"

enum {
	MAX_TASKS = 8,
	MAX_TASK_NAME = 64,
};

typedef struct task {
	bool done;
	char name[MAX_TASK_NAME];
	// Meta
	unid_t id;
	bool is_inserted;
	bool override_array_index;
	bool override_done;
	bool override_name;
} task_t;

typedef struct todo_list {
	task_t tasks[MAX_TASKS];
	int32_t tasks_count;
	// Meta
	unid_t discarded[MAX_TASKS];
	int32_t discarded_count;
	bool has_changes;
} todo_list_t;

unid_t todos_add(todo_list_t* todos, const char* name);

void todos_update_inherited_data(const todo_list_t* base_todos, todo_list_t* derived_todos);

bool edit_todos(todo_list_t* todos, const todo_list_t* base_todos);
