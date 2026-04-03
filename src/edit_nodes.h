#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "utils.h"

enum {
	MAX_NODES = 32,
};

typedef struct node_ref {
	char name[24];
	unid_t id;
	bool is_visible;
	// Meta
	bool is_inserted;
	unid_t base_id;
	bool override_array_index;
	bool override_is_visible;
} node_ref_t;

typedef struct node_ref_array {
	node_ref_t nodes[MAX_NODES];
	int32_t nodes_count;
	// Meta
	unid_t discarded[MAX_NODES];
	int32_t discarded_count;
	bool has_changes;
} node_ref_array_t;

unid_t nodes_add(node_ref_array_t* nodes, const char* name);

void nodes_update_inherited_data(const node_ref_array_t* base_nodes, node_ref_array_t* derived_nodes);

bool edit_nodes(node_ref_array_t* nodes, const node_ref_array_t* base_nodes);
