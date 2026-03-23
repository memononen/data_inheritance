# Data Inheritance

![Image of Data Inheritance protoytype](images/header.png)

This article and source code tries to explain a simple model for data inheritance. Data inheritance means that you can compose data based on existing data, override values, and any changes to the base data will update to the deried data when it is changed. 

Prefabs, prefabs variants, and nested prefabs is the most common applications of data inheritance in game dev. But it does not need to be limited to just that. Any asset or data type could and should support data inheritance. This article tried to lay down some tools to be able to do that.

The whole stack for game data often looks like this:

- **Data Inheritance**: abstraction over how to transfer changes to base data to derived data
- **Hierarchical data**: abstraction over composing data from types (e.g. json, entities & components)
- **Type System and Reflection**: abstraction over types and properties
- **Serialization**: abstraction over how data gets stored one disk or buffers
- **Asset Database**: abstraction over how asset refercing works

This article focuses on the **data inheritance** specifically to make the concepts easy to follow.


## Inheritance Model

We choose to represent the base data and derived data exactly the same way, and meta data, an override flag, to indicate if the value is different from the base data. This allows the derived data to be as fast and easy to access as the base, and it is even possible to snapshot a version an wipeout all the meta data for a release version of asset.

To detect the data changes, there are two common ways to represent the data override: delta (implicit), and flags (explicit). With the delta method, a simple diff is done after the data has been changed to find the items that are different compared to the base data. With the flag method, data is marked as overridden when it is changed.

We choose expliciti flags, since it is just a lot simpler to implement.


## The Override flag

The override flag works simply by having a bit of extra information in the derived data whether a property is overridden or not. At it simplest it could look something like this:

```C
struct {
	CollisionShapeType type;
	float size;
	// Meta
	bool override_type;
	bool override_size;
} collision_shape_t;
```

To set a value, you would do this:

```
void set_size(collision_shape_t* shape, float new_size)
{
	shape->size = new_size;
	if (shape_is_derived(shape))
		shape->override_size;	
}
```

I'm intentionally leaving out the implementation of `shape_is_derived()` as I imagine it is part of the **Hierarchical Data** level depicted above. In essence, the file, or component or node should know if it is derived and where to find the base data.

And to update the data from base to derived, we can simply do this:

```C
void update_inherited_data(const collision_shape_t* base, collision_shape_t* derived)
{
	if (!derived->override_type)
		derived->type = base->type;
	if (!derived->override_size)
		derived->size = base->size;
}
```

That is, we simply just copy over that data that is not overridden.

### Enums Bitflags

Enum flags are a bit special, and can be thought as a set of booleans, so we need an override flag per enum flag. 

Let's say we add `hit_flags`anum bitflags to our collision shape, like this:

```C
typedef enum {
	HIT_STATIC = 1 << 0,
	HIT_DYNAMIC = 1 << 1,
	HIT_SENSOR = 1 << 2,
} hit_flags_t;

struct {
	shape_type_t type;
	float size;
	uint8_t hit_flags;
	// Meta
	bool override_type;
	bool override_size;
	uint8_t override_hit_flags;
} collision_shape_t;
```

We use the same storage type and same flags for the override as we use for the bit flags. We can use cool bitmask trick to copy the overridden bits over without conditions:

```C
void update_inherited_data(const collision_shape_t* base, collision_shape_t* derived)
{
	...
	derived->hit_flags = (derived->hit_flags & derived->override_hit_flags) | (base->hit_flags & ~derived->override_hit_flags);
}
```

That is: `(overridden bits from derived) | (non-overridden bits from base)`.

(Interestingly there is a trick to do that in just 3 instructions: https://realtimecollisiondetection.net/blog/?p=90)

If all of the above looks tedious, it can all be totally automated using a reflection system. In that case you probably want to store the meta data on seprate collection, which makes it even simpler to strip it out if you want to disable the inheritance for your release data.

## Collections

Things get a bit more complicated when we have arrays of data. How do we match the array entries between base and derived data? What if someone removes and item from base, or what if inherited item is deleted in the derived data? Or added, or reordered?

So the main challanges are: how do we relate data between base and derived data, and how to maintain the order of the data. Next we'll look into a couple of different solutions for common use cases.
 
One more thing to note about arrays is that, the arrays we use to store values can have different semantics. Sometimes we want only unique items (a set), sometimes the order matters, sometimes does not, and sometimes the order comes from the data itself.

Here we're going to look 4 differnt types of containers which cover most of the use cases for creative tools and games:
- **Set**: we want a collection of items that are all unique, for example a tag container.
- **Sorted Array**: we store items in an array, but the order comes from sort, for example a curve or gradient.
- **Un-ordered Array**: we store items in an array, but the order does not matter at all, for example node graph nodes
- **Ordered Array**: the items in the array have specific order and we want to maintain it, or we want to keep the order to allow the user to organize the array, for example a todo list. 
- **Map**: each item in the collection is identified by unique data the user can set, for example button bindings table.
- **Array of Objects**: there are special conciderations for objects, for example node hierarchy.

### Which Type to Choose?

In general we should allow users to organize their data. For that reason many of the types in the list are ordered. There are many use case where the ordering does not matter for the same of functionality, but it might be important to organize the data.

Sorted arrays are good for cases where the data is always strictly ordered based on the data. Good example are timelines, gradients and value based lookup tables, where the data's location in time is their natural ordering.

Sets work well for enum type of data, like tags or collection of actual enums. That way they usually have some specific internal ordering, as well as compact data representation. Ordered sets are better choice for other cases where some data will identify an iten uniquely, but we may also have some additional payload, like mapping buttons to actions.

If the ID of the items stored in the array need to be globally unique, the ordered array of objects covers the details for that. The details presented in that section can be also applied to non-sorted array of object.


## Set

Set is a collection of items where each item exists inly once. This makes it really easy to relate the data between base and derived containers.

The most common use for set is gameplay tag collection, aka user-define-mega-enum. Sets whose values are defined externaly can be thought as an extension of enum. 

To follow our override stragegy from previous example, we can define overridable set with an array of tags that should be included, and another array of tags in meta data that describes the tags that has been overridden. In this example we assume that each tag can be represented using an integer, in practice there is some central system that assigns ids to the tags, either via string interning or some other method.

```C
typedef int32_t tag_t; 

typedef struct {
	tag_t tags[MAX_TAGS];
	int32_t tags_count;
	// Meta
	tag_t overrides[MAX_TAGS];
	int32_t overrides_count;
} tag_container_t;
```

If the user adds or removes tag on the derived tag container, then we add unique entry to `overrides` (the same way we added a bit to the enum flags). Also, if the user removes a tag in derived that exists in base, then there is no tag in `tags`, but one exists in `overrides`.

The merging is the same as with enum too, `(overridden tags from derived) | (non-overridden tags from base)`.

```C
void tags_update_inherited_data(const tag_container_t* tags_base, tag_container_t* tags_derived)
{
	tag_t results[MAX_TAGS] = {0};
	int32_t results_count = 0;

	// Overridden tags from derived
	for (int32_t i = 0; i < tags_derived->tags_count; i++) {
		const tag_t tag_id = tags_derived->tags[i];
		if (tags_is_override(tags_derived, tag_id))
			results[results_count++] = tag_id;
	}

	// Non-overridden tags from base
	for (int32_t i = 0; i < tags_base->tags_count; i++) {
		const tag_t tag_id = tags_base->tags[i];
		if (!tags_is_override(tags_derived, tag_id))
			results[results_count++] = tag_id;
	}

	// Store result in derived and sort.
	for (int32_t i = 0; i < results_count; i++)
		tags_derived->tags[i] = results[i];
	tags_derived->tags_count = results_count;

	tags_sort(tags_derived);
}
```

We assume the order of the tags is not significant from their use point of view. Usually we want to know if a tag exists, or iterate over all the tags.

The above merge does not keep the tags in any consistent order. For consistency in the UI, they are sorted so that the tags appear in same order in the picker and in the list.


## Sorted Array

In order to be able to relate data between base and derived we need something to identify the items uniquely. For set, the items we stored in the array were also the unique identifiers. But if we want to store arbitrary data, then we need to resort to additional unique identifier per item. Every time a new item is added to the array, it will be assigned a new unique id.

The simplest thing is a random UID. Using random UIDs allows anyone anywhere to create new IDs without the risk of fearing that two will collide with others. This is important in terms of the inheritance, but also for merging changes via version control. The chance of collision will depend on the size of the UID, 8 bytes might be ok, 16 bytes is quite generally accepted as solid base.

Note: In the example code we will use 2 byte pseudo random IDs so that they are easy to visually debug. Do not use 2 bytes in production!

Since we are also storing other info, we can rethink how we store our overrides. We still need to know which items in the array are created or removed on the derived data. For the items that are new on derived data we could store a flag that indicates that they are added. In addition we will need an array of overrides, list like before, for the items that were removed from the base in the derived data. We will call this array of ids discarded list.

Now our data could look as follows.

```C
typedef struct color_stop {
	float t;
	ImVec4 color;
	// Meta
	uid_t id;
	bool is_inserted;
	bool override_color;
	bool override_t;
} color_stop_t;

typedef struct gradient {
	color_stop_t stops[MAX_COLOR_STOPS];
	int32_t stops_count;
	// Meta
	int32_t discarded[MAX_COLOR_STOPS];
	int32_t discarded_count;
} gradient_t;
```

We are defining data for a gradient, where each color stop stores `is_inserted` if the stop was added to the derived data, and per property overrides, just like in our collision shape example in the beginning.

In addition to the color stops, the gradient also stores discarded list as meta data. This list contains the ids of the color stops in base which were removed.

That is, items with `is_inserted` and the discarded list together forms the override list.

Since the items in the gradient are organized based on their location on the color line, we can use that as the key to sort the items.

```C
void gradient_update_inherited_data(const gradient_t* base_grad, gradient_t* derived_grad)
{
	// Take copy of the derived data before modification.
	color_stop_t result_stops[MAX_COLOR_STOPS];
	int32_t result_stops_count = 0;

	// Keep stops from derived that are inserted.
	for (int32_t i = 0; i < derived_grad->stops_count; i++) {
		const color_stop_t* stop = &derived_grad->stops[i];
		if (stop->is_inserted)
			result_stops[result_stops_count++] = *stop;
	}

	// Keep stops from base which are not overridden, and update property overrides.
	for (int32_t i = 0; i < base_grad->stops_count; i++) {
		const color_stop_t* stop = &base_grad->stops[i];
		// if we have discard override, just skip.
		if (gradient_is_discarded(derived_grad, stop->id))
			continue;
		const int32_t derived_idx = gradient_index_of(derived_grad, stop->id);
		if (derived_idx == INVALID_INDEX) {
			// The item exists only in base, copy over (without base overrides).
			color_stop_t new_stop = {
				.t = stop->t,
				.color = stop->color,
				.id = stop->id,
			};
			result_stops[result_stops_count++] = new_stop;
		} else {
			// If the item exists both in base and derived, we need to merge properties.
			color_stop_t existing_stop = derived_grad->stops[derived_idx];
			// Inherit data
			if (!existing_stop.override_t)
				existing_stop.t = stop->t;
			if (!existing_stop.override_color)
				existing_stop.color = stop->color;
			result_stops[result_stops_count++] = existing_stop;
		}
	}

	// Remove discard overrides that do not exists in base anymore
	for (int32_t i = 0; i < derived_grad->discarded_count; i++) {
		if (gradient_index_of(base_grad, derived_grad->discarded[i]) == INVALID_INDEX) {
			// Could not find in base, remove.
			gradient_remove_discard_at(derived_grad, i);
			i--;
		}
	}

	// Copy results back to derived
	for (int32_t i = 0; i < result_stops_count; i++)
		derived_grad->stops[i] = result_stops[i];
	derived_grad->stops_count = result_stops_count;

	gradient_sort(derived_grad);
}	
```

The merge looks very similar to the tag case, but the second loop is now more complicated as it needs to handle property override too. 

We have also one extra loop, which will remove any discard overrides that cannot exists anymore in the parent. If an item is removed from parent it should not ever come back, since we add unique ids to new items. There is once gotcha, though, if the user modifes base, then saves (which will trigger update), and then does undo, and saves again (update), then it is possible that the same id appears again. Other ways of undoing, like rolling back in version control has similar issues too.

## Un-ordered Array

Unordered array can be implemented just like the sorted array, but just don't sort (doh!). It can be use useful for data that has spatial organization like node graph nodes. Or maybe you sort them by z-index, in which case it's sorted array again.


## Ordered Array

The basic idea of unique ids, per item override flag, and discard list also applies to ordered arrays. The additional detail we need to handle is that output of the update inherited data function will somewhat maintain the order of the items.

There is no perfect solution to this, as the array simply does not capture enough intent from the user. As simple example, if you add new item to the array at the same location both in base and derived, which one should come first? Or if you added 3 items, should the new items interleaved, or should we merge them as clusters in the order they were added in base or derived? Since it is messy, let's put down some things we wish from the ordered merge:

- Maintain order:
	- we assume that modifications in the derived data are in the order the user intended
- Maintain clusters:
	- we assume that items that were modified together should stay together
- Handle common operations predictably:
	- items added to to the end of the derived array should be kept there
	- items added to to the beginning of the derived array should kept there
	- items added to the middle of the base array should appear in their relative position in the derived


The data we want to merge looks very similar to the one we have in the gradient case:

The data for this example is a mapping of buttons to actions: 

```C
typedef struct action {
	button_code_t button;
	action_type_t action;
	// Meta
	uid_t id;
	bool is_inserted;
	bool override_array_index;
	bool override_button;
	bool override_action;
} action_t;

typedef struct action_map {
	action_t actions[MAX_ACTIONS];
	int32_t actions_count;
	// Meta
	uid_t discarded[MAX_ACTIONS];
	int32_t discarded_count;
} action_map_t;
```

The `is_inserted` per action works the same as before, so does the property overrides, there is new override `override_array_index` which indicates that we have modified the (implicit) ordering and we want to keep it in derived. The container is the same as before, we have list of items, and discarded list.


To help us with the ordered merge, we will add an intermediate representation of the items:

```C
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
```

We will make a merge array for both the base and derived data. The `id` is the id of the data to be ordered, we only care about the IDs that are shared by base and derided, so new items in derived array can set the id to invalid. The `base_idx` and `derived_idx` desribe the index of the given item in either array. Fianlly `is_pinned` is used only for the derived array, and it marks items which we want to keep in specific order in the output.

We convert from our list of structs to the merge arrays like this:

```C
void actions_update_inherited_data(const action_map_t* base_actions, action_map_t* derived_actions)
{
	merge_array_t base = {0};
	merge_array_t derived = {0};

	for (int32_t i = 0; i < base_actions->actions_count; i++) {
		merge_array_add(&base, (merge_item_t){
			.id = base_actions->actions[i].id,
			.base_idx = i,
			.derived_idx = INVALID_INDEX,
		});
	}

	for (int32_t i = 0; i < derived_actions->actions_count; i++) {
		const bool is_inserted = derived_actions->actions[i].is_inserted;
		merge_array_add(&derived, (merge_item_t){
			.id = is_inserted ? INVALID_ID : derived_actions->actions[i].id,
			.base_idx = INVALID_INDEX,
			.derived_idx = i,
			.is_pinned = is_inserted || derived_actions->actions[i].override_array_index,
		});
	}
	for (int32_t i = 0; i < derived_actions->discarded_count; i++)
		merge_array_add_discarded(&derived, derived_actions->discarded[i]);

	...
```

For the base merge array we set the `base_idx` and for the derived array we set the `derived_idx`, the first part of the merge process will match the items between the arrays and update the corresponding indices.

Items which have invalid `derived_idx` in the base array mean items that are missing in the derived array, and items with invalid `base_idx` in the derived array means items that are insert in addition to the base items.

The indicas are also later used to quickly relocation the original data when we reoder the actual items.

We set the `is_pinned` to all items in the derived merge array, that we wish to be keep in their current order in the final output. Here we have chosen to keep items that are inserted into the derived, and items that have neem reordered in the derived array. 

The merge magic will update the changes from `base` to the `derived` merge array, and we convert back to the actions like this:

```C
	...
	
	// Do the magic
	merge_array_reconcile(&base, &derived);

	// Copy back adjusted removed items.
	derived_actions->discarded_count = derived.discarded_count;
	for (int32_t i = 0; i < derived.discarded_count; i++)
		derived_actions->discarded[i] = derived.discarded[i];

	// Combine results and inherit properties.
	action_t result_actions[MAX_ACTIONS];
	int32_t result_actions_count = 0;

	for (int32_t i = 0; i < derived.items_count; i++) {
		const int32_t base_idx = derived.items[i].base_idx;
		const int32_t derived_idx = derived.items[i].derived_idx;
		if (derived_idx == INVALID_INDEX) {
			// The item does not exist in derived, create new derived item.
			assert(base_idx != INVALID_INDEX);
			const action_t* base_action = &base_actions->actions[base_idx];
			action_t new_action = {
				.button = base_action->button,
				.action = base_action->action,
				.id = base_action->id,
			};
			assert(result_actions_count < MAX_ACTIONS);
			result_actions[result_actions_count++] = new_action;
		} else {
			// Copy itemfrom original array.
			assert(derived_idx != INVALID_INDEX);
			action_t existing_action = derived_actions->actions[derived_idx];
			// Inherit data
			if (base_idx != INVALID_INDEX && !existing_action.is_inserted) {
				const action_t* base_action = &base_actions->actions[base_idx];
				if (!existing_action.override_button)
					existing_action.button = base_action->button;
				if (!existing_action.override_action)
					existing_action.action = base_action->action;
			}
			assert(result_actions_count < MAX_ACTIONS);
			result_actions[result_actions_count++] = existing_action;
		}
	}

	// Copy results back to derived.
	derived_actions->actions_count = result_actions_count;
	for (int32_t i = 0; i < result_actions_count; i++)
		derived_actions->actions[i] = result_actions[i];
}
```

This is similar to the copy from base loop before, which also contained merging the property overrides.

Now let's take a look at the actual ordered merge:

```C
void merge_array_reconcile_ordered(merge_array_t* base, merge_array_t* derived)
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
	
	...
```

In the first loop we match items between base and derived, and update the indices of the items that match. Then we remove all the discarded items from the base array as if they never existed. After this the derived array contains runs of overridden (pinned, inserted or reodered items), and runs of items that are copied from the base data. The base array contains only the items that we should carry over to the derived data.

The main idea of the next step is to iterate both arrays in parallel, and take overridden (pinned) items from the derived array, and non-overridden items from the base. We are going to use the derived array as template. If there is a run of pinned items in the derived array, we will take them. For a run of non-pinned items, we take equal amount of items from base. If we encounter new item while we are adding items from base, they will be added "for free" and do not reduce the count we set to copy from.

```C
	...
	
	// Iterate over base and derived, picking pinned sections from derived, 
	// and non-pinned or new base items from base.
	merge_item_t results[MAX_ITEMS];
	int32_t results_count = 0;
	int32_t cur_base_idx = 0;
	int32_t cur_derived_idx = 0;
	while (cur_base_idx < base->items_count || cur_derived_idx < derived->items_count) {
		if (cur_derived_idx < derived->items_count && derived->items[cur_derived_idx].is_pinned) {
			// Modified section, keep as is
			while (cur_derived_idx < derived->items_count 
					&& derived->items[cur_derived_idx].is_pinned) {
				assert(results_count+1 <= MAX_ITEMS);
				results[results_count++] = derived->items[cur_derived_idx];
				cur_derived_idx++;
			}
		} else {
			// Skip unmodified section from derived, keep track how many we skipped.
			int32_t count = 0;
			while (cur_derived_idx < derived->items_count 
					&& !derived->items[cur_derived_idx].is_pinned) {
				cur_derived_idx++;
				count++;
			}
			// Add skipped amount of unmodified items, or adjacent items that were added to the base.
			while (cur_base_idx < base->items_count 
					&& (count > 0 || base->items[cur_base_idx].derived_id == INVALID_ID)) {

				assert(results_count+1 <= MAX_ITEMS);
				results[results_count++] = base->items[cur_base_idx];
				// New items do not count against the quota.
				if (base->items[cur_base_idx].derived_id != INVALID_ID)
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
```

This will keep the order and clusters of overridden derived data, also making sure that any item added to the base will land in the nearest non-overridden run in the derived data. Reordering the items in base will not affect the ordering of the derived data.

### Alternatives

Fracitonal indexing is a quite popular option for array ordering, especially in the web space. The idea is to assing and index (that is fractional) to each item, and use sorting to reorder the items after merge. When a new item is inserted, it's index is midway the adjacent items. This method has some down sides like: empty ranges (when two items are added to same location) and unbounded index length, and item interleaving (items added in same location will get mixed up). 

Longest edit subsequence or longest increasing subsequence could be used to improve the alignment of the merged arrays. Particularly when items are reodered in the base array. I have tested with quite a few alignment options, and they work most of the time, but sometimes the results can be quite unpredictable, particularly if an item was rerdered to opposite end. Quite a bit of heuristics might be needed to get stable feeling results.

## Map

Maps and Set both are _exceptionally_ hard to handle in the user interface, even without any data inheritance.

For example, we might have a mapping from button codes to actions, say we start with this map:

- Button A -> Jump
- Button B -> Fire

Now if we wanted to switch over the buttons, might first change the first item's key from A to B. But that would override the existing B and we would loose data. Sets have similar issue, for that reason they are better left for data that can be thought as tags containers and such.

One option to implement maps is to just treat them as ordered lists, add validation, and lookup index. Ordered list had predictable UI workflows, and you can use validation to mark duplicate entries. A separate lookup index allows map-like fast query, and graceful handling of duplicates (e.g. do not add duplicates, or last item wins, etc).


## Array of Objects

So far we have deal with items which are locally identified per container. The ids have influence only across the chain of derived assets.

There are cases where the ID uniquely represent an object in the whole system or within an asset. In such cases we need to store extra data to match the ids between the base data an derived data, we call this addition reference `base_id`.

![Inheritance](images/inheritance.svg)

The `base_id` may also be used for other purpose than correlating data between related arrays. Another object could be also used as a template for newly created item. Nested prefabs are good example of such setup. This changes the semantics of the `base_id` since now it describes the inheritance chain, not just array relation.

By using the `is_inserted` flag we can differentiate the cases where we added a new item that is derived from some object, vs. cases where we have inherited an item in an array.

```C
typedef struct node_ref {
	char name[24];
	int32_t id;
	// Meta
	bool is_inserted;
	int32_t base_id;
	bool override_array_index;
} node_ref_t;

typedef struct node_ref_array {
	node_ref_t nodes[MAX_NODES];
	int32_t nodes_count;
	// Meta
	int32_t discarded[MAX_NODES];
	int32_t discarded_count;
} node_ref_array_t;
```


For merging object arrays, we are going to use the same merge array as before:

```C
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
		const bool is_inserted = derived_nodes->nodes[i].is_inserted;
		merge_array_add(&derived, (merge_item_t){
			.id = is_inserted ? INVALID_ID : derived_nodes->nodes[i].base_id,
			.base_idx = INVALID_INDEX,
			.derived_idx = i,
			.is_pinned = is_inserted || derived_nodes->nodes[i].override_array_index,
		});
	}
	for (int32_t i = 0; i < derived_nodes->discarded_count; i++)
		merge_array_add_discarded(&derived, derived_nodes->discarded[i]);

	...
```

The object ids in the base and derived do not match, but we can use the `id` from the base array, `base_id` from the derived array to match the items to merge. Note, how we only set the id for nodes that are _not_ created in the derived data, even if they might have calid `base_id`. This allows a new item inserted in the derived array to inherit from any object in the base.

## The UI

![Image of Data Inheritance protoytype](images/footer.png)

The challenge for the override visualization is that it needs to work with many different types of widgets and contexts. A fairly common way to represent a overridden property is drawing a colored (usually blue) vertical line at the begining of the changed line. It is not too distracting, yet quick to recognize at glance. The simple shape is quite easy to slap next to just about any widget.

We should also have a way to indicate that there are overrides within a data container (object, array, etc). This allows to quickly see where the overriden data is even if the actual data is outside the view or inside collapsed UI.

The override indicator also should be granular enough that if you have laid out multiple widgets on the same row, each of them can be highlighted out separately.

One challange I faced in this prototype was how to apply the indicator to markers, like keyframes. Many options I tried felt too much like the marker was selected. I eventually settled in a horizontal line over the marker, as it did not look like anything that felt like it had meaning on the timeline.

In addition to the indicator we also need a way to revert the changes. For widget rows, the easiest is to have a button, or an option in right click menu. Some applications seem to combine the revert button and per row override indicator, which can also be a good idea.

Removed items need extra care, since they are not represented in the UI. I chose to include simple red indicator next to the item count to signal that items have been removed from the container. The indicator also has quick preview of the items as tooltip.

The revert button on the container header allows all the changes to be reverted, or more granular way to revert the removed items.

The revert UI can get as complex as the rest of the UI. We are essentially building a tool that handles the invisible connections between two documents.

## Conclusion

A robust and efficient solution for data inheritance can be hard to implement, since many decisions require making decisions across the whole tech stack from low level representation to the UI. I hope this article could provide some guidance, and maybe the prototype can be used to test some ideas out.