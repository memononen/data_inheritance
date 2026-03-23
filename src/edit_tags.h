#pragma once

#include "dcimgui.h"

typedef int32_t tag_t;

typedef struct tag_desc {
	const char* name;
	ImU32 color;
	tag_t id;
} tag_desc_t;

const tag_desc_t* get_tag_by_id(const tag_desc_t* tag_descs, int32_t tag_descs_count, tag_t tag_id);

enum {
	MAX_TAGS = 16
};

typedef struct {
	tag_t tags[MAX_TAGS];
	int32_t tags_count;
	// Meta
	tag_t overrides[MAX_TAGS];
	int32_t overrides_count;
	bool has_changes;
} tag_container_t;

void tags_add(tag_container_t* tags, tag_t tag_id);
void tags_update_inherited_data(const tag_container_t* tags_base, tag_container_t* tags_derived);
bool edit_tags(tag_container_t* tags, bool is_derived);
