#include "utils.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>


static int32_t merge_array_find_by_base_id(const merge_array_t* merge, uid_t id)
{
	for (int32_t i = 0; i < merge->items_count; i++) {
		if (merge->items[i].id == id)
			return i;
	}
	return INVALID_INDEX;
}

static void merge_array_remove_at(merge_array_t* merge, int32_t idx)
{
	merge->items_count--;
	for (int32_t j = idx; j < merge->items_count; j++)
		merge->items[j] = merge->items[j + 1];
}

void merge_array_add(merge_array_t* merge, merge_item_t new_item)
{
	if (merge->items_count >= MAX_ITEMS)
		return;
	merge_item_t* item = &merge->items[merge->items_count++];
	*item = new_item;
}

void merge_array_add_discarded(merge_array_t* merge, uid_t removed_id)
{
	if (merge->discarded_count >= MAX_ITEMS)
		return;
	merge->discarded[merge->discarded_count++] = removed_id;
}

void merge_array_remove_discarded_at(merge_array_t* merge, int32_t idx)
{
	merge->discarded_count--;
	for (int32_t i = idx; i < merge->discarded_count; i++)
		merge->discarded[i] = merge->discarded[i + 1];
}

void merge_array_reconcile(merge_array_t* base, merge_array_t* derived)
{
	// Match derived to base.
	for (int32_t i = 0; i < derived->items_count; i++) {
		merge_item_t* item = &derived->items[i];
		// If this derived item has matching base item, try to match them across.
		item->base_idx = INVALID_INDEX;
		if (item->id != INVALID_ID) {
			item->base_idx = merge_array_find_by_base_id(base, item->id);
			if (item->base_idx == INVALID_INDEX) {
				// The matching item in base was removed, remove derived item too.
				merge_array_remove_at(derived, i);
				i--;
				continue;
			}
			if (item->is_pinned) {
				// Remove pinned items from base, as they are always picked from derived.
				merge_array_remove_at(base, item->base_idx);
			} else {
				// Match related derived item in base.
				base->items[item->base_idx].derived_idx = item->derived_idx;
			}
		} else {
			// This item does not exist on base, it's inserted to derived and must be pinned.
			item->is_pinned = true;
		}
	}

	// Remove items from the base, that are removed in derived.
	for (int32_t i = 0; i < derived->discarded_count; i++) {
		// Validate removes
		const int32_t base_idx = merge_array_find_by_base_id(base, derived->discarded[i]);
		if (base_idx == INVALID_INDEX) {
			// The remove is not found in parent, remove it.
			merge_array_remove_discarded_at(derived, i);
			i--;
			continue;
		}
		// Item is removed, do not try to merge it.
		merge_array_remove_at(base, base_idx);
	}

	// Iterate over base and derived, picking pinned sections from derived, and non-pinned or new base items from base.
	merge_item_t results[MAX_ITEMS];
	int32_t results_count = 0;
	int32_t cur_base_idx = 0;
	int32_t cur_derived_idx = 0;
	while (cur_base_idx < base->items_count || cur_derived_idx < derived->items_count) {
		if (cur_derived_idx < derived->items_count && derived->items[cur_derived_idx].is_pinned) {
			// Modified section, keep as is
			while (cur_derived_idx < derived->items_count && derived->items[cur_derived_idx].is_pinned) {
				assert(results_count+1 <= MAX_ITEMS);
				results[results_count++] = derived->items[cur_derived_idx];
				cur_derived_idx++;
			}
		} else {
			// Skip unmodified section from derived, keep track how many we skipped.
			int32_t count = 0;
			while (cur_derived_idx < derived->items_count && !derived->items[cur_derived_idx].is_pinned) {
				cur_derived_idx++;
				count++;
			}
			// Add skipped amount of unmodified items, or adjacent items that were added to the base.
			while (cur_base_idx < base->items_count && (count > 0 || base->items[cur_base_idx].derived_idx == INVALID_INDEX)) {
				assert(results_count+1 <= MAX_ITEMS);
				results[results_count++] = base->items[cur_base_idx];
				// New items do not count against the quota.
				if (base->items[cur_base_idx].derived_idx != INVALID_INDEX)
					count--;
				cur_base_idx++;
			}
		}
	}

	// Copy result to derived
	derived->items_count = results_count;
	for (int32_t i = 0; i < results_count; i++)
		derived->items[i] = results[i];
}

// Using toy 16 bit ID generator so that we have random IDs, but easy reference IDs. For toy use only!
int32_t gen_id(void)
{
	static uint16_t xs = 7;
	xs ^= xs << 7;
	xs ^= xs >> 9;
	xs ^= xs << 8;
	return (int32_t)xs;
}

static void enc62(uint16_t a, char* str)
{
	static const char* enc_table = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	static const int32_t iters = 3; // 3 iterations get us upt to 62^3 = 238328.
	for (int32_t i = 0; i < iters; i++) {
		str[iters-1-i] = enc_table[a % 62];
		a /= 62;
	}
	str[iters] = '\0';
}

const char* id_to_str(int32_t id)
{
	#define SLOT_COUNT 8
	static char id_str[4*SLOT_COUNT];	// reserve multiple slots, so that multiple calls can be used in one printf.
	static int32_t slot = 0;
	char* slot_str = &id_str[slot*4];
	slot = (slot+1) % SLOT_COUNT;

	enc62((uint16_t)id, slot_str);
	return slot_str;
}


const char* gen_name(const char* name_base)
{
	static int32_t name_idx = 0;
	static char name[128];
	snprintf(name, sizeof(name), "%s %c", name_base, 'A' + (name_idx % 27));
	name_idx++;
	return name;
}
