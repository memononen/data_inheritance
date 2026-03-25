#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "dcimgui.h"
#include "utils.h"

enum {
	MAX_COLOR_STOPS = 16
};

typedef struct color_stop {
	float pos;
	ImVec4 color;
	// Meta
	uid_t id;
	bool is_inserted;
	bool override_color;
	bool override_pos;
} color_stop_t;

typedef struct gradient {
	color_stop_t stops[MAX_COLOR_STOPS];
	int32_t stops_count;
	// Meta
	uid_t discarded[MAX_COLOR_STOPS];
	int32_t discarded_count;
	bool has_changes;
} gradient_t;

uid_t gradient_add(gradient_t* grad, float t, ImVec4 color);

void gradient_update_inherited_data(const gradient_t* base_grad, gradient_t* derived_grad);

bool edit_gradient(gradient_t* grad, const gradient_t* base_grad);
