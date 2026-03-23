#pragma once

#include "dcimgui.h"

typedef enum {
	HIT_STATIC = 1 << 0,
	HIT_DYNAMIC = 1 << 1,
	HIT_SENSOR = 1 << 2,
} hit_flags_t;

typedef enum {
	SHAPE_CIRCLE,
	SHAPE_TRIANGLE,
	SHAPE_SQUARE,
} shape_type_t;

typedef struct {
	shape_type_t shape;
	float size;
	uint8_t hit_flags;
	// Meta
	bool override_size;
	bool override_shape;
	uint8_t override_hit_flags;
	bool has_changes;
} collision_shape_t;

void shape_update_inherited_data(collision_shape_t* base, collision_shape_t* derived);

bool edit_collosion_shape(collision_shape_t* shape, bool is_derived);
