#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <math.h>

#include "tower-aoi.h"
#include "hash.h"
#include "hash_witness.h"

#ifdef _WIN32
#define inline __inline
#endif

#define TYPE_ENTITY 	0
#define TYPE_TRIGGER  	1

#define RESTORE_WITNESS
#define RESTORE_VISIBLE

typedef struct location {
	float x;
	float z;
} location_t;

typedef struct object {
	int uid;
	int id;
	uint8_t type;

	location_t local;

#ifdef RESTORE_WITNESS
	hash_set_t* witness;
#endif
#ifdef RESTORE_VISIBLE
	hash_set_t* visible;
#endif

	struct object* next;
	struct object* prev;

	int range;
} object_t;

typedef struct region {
	int32_t begin_x;
	int32_t begin_z;
	int32_t end_x;
	int32_t end_z;
} region_t;

typedef struct tower {
	struct object head;
	hash_t* hash;
} tower_t;

typedef struct aoi {
	uint32_t width;
	uint32_t height;

	uint32_t cell;

	tower_t** towers;
	uint32_t tower_x;
	uint32_t tower_z;

	object_t* pool;
	size_t max;

	object_t* freelist;
} aoi_t;


static inline void translate(aoi_t* aoi, location_t* in, location_t* out) {
	out->x = in->x / aoi->cell;
	out->z = in->z / aoi->cell;
}

static inline void get_region(aoi_t* aoi, location_t* local, region_t* out, uint32_t range) {
	if (local->x - range < 0) {
		out->begin_x = 0;
		out->end_x = local->x + range;
		if (out->end_x < 0) {
			out->end_x = 0;
		} else if (out->end_x >= aoi->tower_x) {
			out->end_x = aoi->tower_x - 1;
		}
	} else if (local->x + range >= aoi->tower_x) {
		out->end_x = aoi->tower_x - 1;
		out->begin_x = local->x - range;
		if (out->begin_x < 0) {
			out->begin_x = 0;
		} else if (out->begin_x >= aoi->tower_x) {
			out->begin_x = aoi->tower_x - 1;
		}
	} else {
		out->begin_x = local->x - range;
		out->end_x = local->x + range;
	}

	if (local->z - range < 0) {
		out->begin_z = 0;
		out->end_z = local->z + range;
		if (out->end_z < 0) {
			out->end_z = 0;
		} else if (out->end_z >= aoi->tower_z) {
			out->end_z = aoi->tower_z - 1;
		}
	} else if (local->z + range >= aoi->tower_z) {
		out->end_z = aoi->tower_z - 1;
		out->begin_z = local->z - range;
		if (out->begin_z < 0) {
			out->begin_z = 0;
		} else if (out->begin_z >= aoi->tower_z) {
			out->begin_z = aoi->tower_z - 1;
		}
	} else {
		out->begin_z = local->z - range;
		out->end_z = local->z + range;
	}
}

static inline object_t* new_object(aoi_t* aoi, int uid, int type, float x, float z) {
	if (aoi->freelist == NULL) {
		return NULL;
	}
	object_t* obj = aoi->freelist;
	aoi->freelist = obj->next;
	obj->prev = obj->next = NULL;
	obj->range = 0;
	obj->uid = uid;
	obj->type = type;
	obj->local.x = x;
	obj->local.z = z;
	return obj;
}

static inline void free_object(aoi_t* aoi, object_t* obj) {
	obj->next = aoi->freelist;
	aoi->freelist = obj;
}

static inline void list_add(object_t* head, object_t* object) {
	object_t* prev = head->prev;
	prev->next = object;
	object->next = head;
	object->prev = prev;
	head->prev = object;
}

static inline void list_remove(object_t* object) {
	object_t* next = object->next;
	object_t* prev = object->prev;
	next->prev = prev;
	prev->next = next;
	object->prev = NULL;
	object->next = NULL;
}

static inline struct object* get_object(aoi_t* aoi, int id, int type) {
	if (id < 0 || id >= aoi->max)
		return NULL;

	struct object* result = &aoi->pool[id];
	if (result->type != type)
		return NULL;

	return result;
}

static inline void assert_position(aoi_t* aoi, float x, float z) {
	if (x < 0 || z < 0 || x > aoi->width || z > aoi->height) {
		assert(0);
	}
}

int create_entity(aoi_t* aoi, int uid, float x, float z, enter_func func, void* ud) {
	assert_position(aoi, x, z);

	object_t* entity = new_object(aoi, uid, TYPE_ENTITY, x, z);

	location_t out;
	translate(aoi, &entity->local, &out);

	tower_t* tower = &aoi->towers[(uint32_t)out.x][(uint32_t)out.z];

	list_add(&tower->head, entity);

	entity->witness = hash_set_new();
	khiter_t k;
	object_t* other;
	hash_foreach(tower->hash, k, other, {
		if (other->uid != entity->uid) {
			func(entity->uid, other->uid, ud);
			hash_set_put(entity->witness, other->uid, entity->uid, "entity witness");
			hash_set_put(other->visible, entity->uid, other->uid, "trigger visible");
		}
	});

	return entity->id;
}

void remove_entity(aoi_t* aoi, int id, leave_func func, void* ud) {
	object_t* entity = get_object(aoi, id, TYPE_ENTITY);

	location_t out;
	translate(aoi, &entity->local, &out);

	tower_t* tower = &aoi->towers[(uint32_t)out.x][(uint32_t)out.z];

	list_remove(entity);

	khiter_t k;
	object_t* other;
	hash_foreach(tower->hash, k, other, {
		if (other->uid != entity->uid) {
			func(entity->uid, other->uid, ud);
			hash_set_del(entity->witness, other->uid, entity->uid, "entity witness");
		}
	});

	free_object(aoi, entity);
	return;
}

void move_entity(aoi_t* aoi, int id, float nx, float nz, enter_func enter_func, void* enter_ud, leave_func leave_func, void* leave_ud) {
	assert_position(aoi, nx, nz);

	object_t* entity = get_object(aoi, id, TYPE_ENTITY);

	location_t out;
	translate(aoi, &entity->local, &out);
	tower_t* otower = &aoi->towers[(uint32_t)out.x][(uint32_t)out.z];

	entity->local.x = nx;
	entity->local.z = nz;
	translate(aoi, &entity->local, &out);
	tower_t* ntower = &aoi->towers[(uint32_t)out.x][(uint32_t)out.z];

	if (ntower == otower) {
		return;
	}

	list_remove(entity);

	list_add(&ntower->head, entity);

	object_t leave;
	leave.next = leave.prev = &leave;
	object_t enter;
	enter.next = enter.prev = &enter;
	khiter_t k;
	object_t* other;

	hash_foreach(otower->hash, k, other, {
		(void)k;
		if (other->uid != entity->uid) {
			list_add(&leave, other);
		}
	});

	hash_foreach(ntower->hash, k, other, {
		if (other->uid != entity->uid) {
			if (other->next != NULL || other->prev != NULL) {
				list_remove(other);
			} else {
				list_add(&enter, other);
			}
		}
	});

	object_t* cursor = leave.next;
	for (; cursor != &leave;) {
		object_t* obj = cursor;
		leave_func(entity->uid, obj->uid, leave_ud);
		cursor = cursor->next;
		obj->next = obj->prev = NULL;
		hash_set_del(entity->witness, obj->uid, entity->uid, "entity witness");
		hash_set_del(obj->visible, entity->uid, obj->uid, "trigger visible");
	}

	cursor = enter.next;
	for (; cursor != &enter;) {
		object_t* obj = cursor;
		cursor = cursor->next;
		enter_func(entity->uid, obj->uid, enter_ud);
		obj->next = obj->prev = NULL;
		hash_set_put(entity->witness, obj->uid, entity->uid, "entity witness");
		hash_set_put(obj->visible, entity->uid, obj->uid, "trigger visible");
	}
}

int create_trigger(aoi_t* aoi, int uid, float x, float z, int range, enter_func func, void* ud) {
	assert_position(aoi, x, z);

	object_t* trigger = new_object(aoi, uid, TYPE_TRIGGER, x, z);
	trigger->visible = hash_set_new();

	trigger->range = range;

	location_t out;
	translate(aoi, &trigger->local, &out);

	region_t region;
	get_region(aoi, &out, &region, range);

	uint32_t x_index;
	for (x_index = region.begin_x; x_index <= region.end_x; x_index++) {
		uint32_t z_index;
		for (z_index = region.begin_z; z_index <= region.end_z; z_index++) {
			tower_t* tower = &aoi->towers[x_index][z_index];

			hash_set(tower->hash, trigger->uid, trigger);

			struct object* cursor = tower->head.next;
			for (; cursor != &tower->head; cursor = cursor->next) {
				if (cursor->uid != trigger->uid) {
					func(trigger->uid, cursor->uid, ud);
					hash_set_put(cursor->witness, trigger->uid, cursor->uid, "entity witness");
					hash_set_put(trigger->visible, cursor->uid, trigger->uid, "trigger visible");

				}
			}
		}
	}
	return trigger->id;
}

void remove_trigger(aoi_t* aoi, int id) {
	object_t* trigger = get_object(aoi, id, TYPE_TRIGGER);

	location_t out;
	translate(aoi, &trigger->local, &out);

	region_t region;
	get_region(aoi, &out, &region, trigger->range);

	uint32_t x_index;
	for (x_index = region.begin_x; x_index <= region.end_x; x_index++) {
		uint32_t z_index;
		for (z_index = region.begin_z; z_index <= region.end_z; z_index++) {
			tower_t* tower = &aoi->towers[x_index][z_index];
			hash_del(tower->hash, trigger->uid);
		}
	}

	free_object(aoi, trigger);
}

void move_trigger(aoi_t* aoi, int id, float nx, float nz, enter_func enter, void* enter_ud, leave_func leave, void* leave_ud) {
	assert_position(aoi, nx, nz);

	object_t* trigger = get_object(aoi, id, TYPE_TRIGGER);

	location_t oout;
	translate(aoi, &trigger->local, &oout);

	location_t nout;
	trigger->local.x = nx;
	trigger->local.z = nz;
	translate(aoi, &trigger->local, &nout);

	if (oout.x == nout.x && oout.z == nout.z) {
		return;
	}

	region_t oregion;
	get_region(aoi, &oout, &oregion, trigger->range);

	region_t nregion;
	get_region(aoi, &nout, &nregion, trigger->range);

	uint32_t x_index;
	for (x_index = oregion.begin_x; x_index <= oregion.end_x; x_index++) {
		uint32_t z_index;
		for (z_index = oregion.begin_z; z_index <= oregion.end_z; z_index++) {
			if (x_index >= nregion.begin_x && x_index <= nregion.end_x && z_index >= nregion.begin_z && z_index <= nregion.end_z) {
				continue;
			}
			tower_t* tower = &aoi->towers[x_index][z_index];
			hash_del(tower->hash, trigger->uid);

			struct object* cursor = tower->head.next;
			for (; cursor != &tower->head; cursor = cursor->next) {
				if (cursor->uid != trigger->uid) {
					leave(trigger->uid, cursor->uid, leave_ud);
					hash_set_del(cursor->witness, trigger->uid, cursor->uid, "entity witness");
					hash_set_del(trigger->visible, cursor->uid, trigger->uid, "trigger visible");
				}
			}
		}
	}


	for (x_index = nregion.begin_x; x_index <= nregion.end_x; x_index++) {
		uint32_t z_index;
		for (z_index = nregion.begin_z; z_index <= nregion.end_z; z_index++) {
			if (x_index >= oregion.begin_x && x_index <= oregion.end_x && z_index >= oregion.begin_z && z_index <= oregion.end_z) {
				continue;
			}
			tower_t* tower = &aoi->towers[x_index][z_index];

			hash_set(tower->hash, trigger->uid, trigger);

			struct object* cursor = tower->head.next;
			for (; cursor != &tower->head; cursor = cursor->next) {
				if (cursor->uid != trigger->uid) {
					enter(trigger->uid, cursor->uid, enter_ud);
					hash_set_put(cursor->witness, trigger->uid, cursor->uid, "entity witness");
					hash_set_put(trigger->visible, cursor->uid, trigger->uid, "trigger visible");
				}
			}
		}
	}
}

void release_aoi(aoi_t* aoi) {
	uint32_t x;
	for (x = 0; x < aoi->tower_x; x++) {
		uint32_t z;
		for (z = 0; z < aoi->tower_z; z++) {
			tower_t* tower = &aoi->towers[x][z];
			hash_free(tower->hash);
		}
		free(aoi->towers[x]);
	}
	free(aoi->towers);
	free(aoi->pool);
	free(aoi);
}

aoi_t* create_aoi(int max, int width, int height, int cell) {
	aoi_t* aoi_ctx = (aoi_t*)malloc(sizeof(aoi_t));
	memset(aoi_ctx, 0, sizeof(*aoi_ctx));

	aoi_ctx->max = max;

	aoi_ctx->pool = malloc(sizeof(object_t)* aoi_ctx->max);
	memset(aoi_ctx->pool, 0, sizeof(object_t)* aoi_ctx->max);

	aoi_ctx->width = width;
	aoi_ctx->height = height;
	aoi_ctx->cell = cell;

	if (aoi_ctx->width < aoi_ctx->cell)
		aoi_ctx->cell = aoi_ctx->width;

	if (aoi_ctx->height < aoi_ctx->cell)
		aoi_ctx->cell = aoi_ctx->height;

	aoi_ctx->tower_x = aoi_ctx->width / aoi_ctx->cell + 1;
	aoi_ctx->tower_z = aoi_ctx->height / aoi_ctx->cell + 1;

	size_t i;
	for (i = 0; i < aoi_ctx->max; i++) {
		object_t* obj = &aoi_ctx->pool[i];
		obj->id = i;
		obj->next = aoi_ctx->freelist;
		aoi_ctx->freelist = obj;
	}

	aoi_ctx->towers = malloc(aoi_ctx->tower_x * sizeof(*aoi_ctx->towers));
	uint32_t x;
	for (x = 0; x < aoi_ctx->tower_x; x++) {
		aoi_ctx->towers[x] = malloc(aoi_ctx->tower_z * sizeof(tower_t));
		memset(aoi_ctx->towers[x], 0, aoi_ctx->tower_z * sizeof(tower_t));
		uint32_t z;
		for (z = 0; z < aoi_ctx->tower_z; z++) {
			tower_t* tower = &aoi_ctx->towers[x][z];
			tower->head.prev = &tower->head;
			tower->head.next = &tower->head;
			tower->hash = hash_new();
		}
	}
	return aoi_ctx;
}

void get_witness(struct aoi* aoi, int id, callback_func func, void* ud) {
	object_t* entity = get_object(aoi, id, TYPE_ENTITY);

	location_t out;
	translate(aoi, &entity->local, &out);

	tower_t* tower = &aoi->towers[(uint32_t)out.x][(uint32_t)out.z];

	khiter_t k;
	object_t* other;

	hash_foreach(tower->hash, k, other, {
		(void)k;
		if (other->uid != entity->uid) {
			func(other->uid, ud);
		}
	});
}

void get_visible(struct aoi* aoi, int id, callback_func func, void* ud) {
	object_t* trigger = get_object(aoi, id, TYPE_TRIGGER);

	location_t out;
	translate(aoi, &trigger->local, &out);

	region_t region;
	get_region(aoi, &out, &region, trigger->range);

	uint32_t x_index;
	for (x_index = region.begin_x; x_index <= region.end_x; x_index++) {
		uint32_t z_index;
		for (z_index = region.begin_z; z_index <= region.end_z; z_index++) {
			tower_t* tower = &aoi->towers[x_index][z_index];
			struct object* cursor = tower->head.next;
			for (; cursor != &tower->head; cursor = cursor->next) {
				if (trigger->uid != cursor->uid) {
					func(cursor->uid, ud);
				}
			}
		}
	}
}

void foreach_entity(aoi_t* aoi, foreach_entity_func func, void* ud) {
	uint32_t x;
	for (x = 0; x < aoi->tower_x; x++) {
		uint32_t z;
		for (z = 0; z < aoi->tower_z; z++) {
			tower_t* tower = &aoi->towers[x][z];
			struct object* cursor = tower->head.next;
			for (; cursor != &tower->head; cursor = cursor->next) {
				object_t* entity = cursor;
				func(entity->uid, entity->local.x, entity->local.z, ud);
			}
		}
	}
}

void foreach_trigger(aoi_t* aoi, foreach_trigger_func func, void* ud) {
	hash_t* hash = hash_new();

	uint32_t x;
	for (x = 0; x < aoi->tower_x; x++) {
		uint32_t z;
		for (z = 0; z < aoi->tower_z; z++) {
			tower_t* tower = &aoi->towers[x][z];
			khiter_t k;
			object_t* trigger;
			hash_foreach(tower->hash, k, trigger, {
				(void)k;
				if (!hash_has(hash, trigger->uid)) {
					hash_set(hash, trigger->uid, trigger);
				}
			});
		}
	}

	khiter_t k;
	object_t* trigger;
	hash_foreach(hash, k, trigger, {
		(void)k;
		func(trigger->uid, trigger->local.x, trigger->local.z, trigger->range, ud);
	});
	hash_free(hash);
}
