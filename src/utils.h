#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef int32_t uid_t;

static inline float clampf(float x, float mn, float mx)
{
	return x < mn ? mn : (x > mx ? mx : x);
}

enum {
	INVALID_ID = 0,
	INVALID_INDEX = -1,
	INSERT_END = -1,
};

enum {
	MAX_ITEMS = 32,
};

typedef struct merge_item {
	uid_t id;
	int32_t base_idx;
	int32_t derived_idx;
	bool is_pinned;
} merge_item_t;

typedef struct array_merge {
	merge_item_t items[MAX_ITEMS];
	int32_t items_count;
	uid_t discarded[MAX_ITEMS];
	int32_t discarded_count;
} merge_array_t;

void merge_array_add(merge_array_t* merge, merge_item_t new_item);
void merge_array_add_discarded(merge_array_t* merge, uid_t removed_id);
void merge_array_reconcile(merge_array_t* base, merge_array_t* derived);

uid_t gen_id(void);
const char* id_to_str(uid_t id);

const char* gen_name(const char* name_base);

typedef enum {
	COMMAND_NONE = 0,
	COMMAND_INSERT_AT,
	COMMAND_REMOVE_AT,
	COMMAND_REORDER_AT,
	COMMAND_REVERT_ALL,
	COMMAND_REVERT_DISCARDED,
	COMMAND_REVERT_ALL_DISCARDED,
	COMMAND_REVERT_AT,
	COMMAND_BREAK_INHERITANCE_AT,
} command_type_t;

typedef struct command {
	command_type_t type;
	int32_t idx;
	int32_t target_idx;
	int32_t base_id;
	float pos;
} command_t;

static inline command_t command_make_insert_at(int32_t idx)
{
	return (command_t) {
		.type = COMMAND_INSERT_AT,
		.idx = idx,
		.base_id = 0,
	};
}

static inline command_t command_make_insert_at_base_id(int32_t idx, int32_t base_id)
{
	return (command_t) {
		.type = COMMAND_INSERT_AT,
		.idx = idx,
		.base_id = base_id,
	};
}

static inline command_t command_make_insert_at_pos(float t)
{
	return (command_t) {
		.type = COMMAND_INSERT_AT,
		.pos = t,
	};
}

static inline command_t command_make_remove_at(int32_t idx)
{
	return (command_t) {
		.type = COMMAND_REMOVE_AT,
		.idx = idx,
	};
}

static inline command_t command_make_reorder_at(int32_t from_idx, int32_t to_idx)
{
	return (command_t) {
		.type = COMMAND_REORDER_AT,
		.idx = from_idx,
		.target_idx = to_idx,
	};
}

static inline command_t command_make_revert_all(void)
{
	return (command_t) {
		.type = COMMAND_REVERT_ALL,
	};
}

static inline command_t command_make_revert_all_discarded(void)
{
	return (command_t) {
		.type = COMMAND_REVERT_ALL_DISCARDED,
	};
}

static inline command_t command_make_revert_discarded(int32_t base_id)
{
	return (command_t) {
		.type = COMMAND_REVERT_DISCARDED,
		.base_id = base_id,
	};
}

static inline command_t command_make_revert_at(int32_t idx)
{
	return (command_t) {
		.type = COMMAND_REVERT_AT,
		.idx = idx,
	};
}

static inline command_t command_make_break_inheritance_at(int32_t idx)
{
	return (command_t) {
		.type = COMMAND_BREAK_INHERITANCE_AT,
		.idx = idx,
	};
}
