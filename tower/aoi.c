#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <stdint.h>

#include "aoi.h"
#include "khash.h"
#include "hash.h"

#ifdef _WIN32
#define inline __inline
#endif

#define TYPE_ENTITY 	0
#define TYPE_TRIGGER  	1

typedef struct location {
	float x;
	float z;
} location_t;

typedef struct object {
	int uid;
	int id;
	uint8_t type;

	union {
		int range;
		struct {
			struct object* link_next;
			struct object* link_prev;
		} link_node;
	} param;

	location_t local;
	struct object* next;
	struct object* prev;
	callback_func func;
	void* ud;
} object_t;

typedef struct region {
	int32_t begin_x;
	int32_t begin_z;
	int32_t end_x;
	int32_t end_z;
} region_t;

typedef struct tower {
	struct object* head;
	struct object* tail;
	hash_t* hash;
} tower_t;

typedef struct aoi {
	uint32_t width;
	uint32_t height;

	uint32_t cell;

	uint32_t tower_x;
	uint32_t tower_z;

	object_t* pool;
	size_t max;

	object_t* freelist;

	tower_t** towers;
} aoi_t;


static inline void
translate(aoi_t* aoi, location_t* in, location_t* out) {
	out->x = in->x / aoi->cell;
	out->z = in->z / aoi->cell;
}

static inline void
get_region(aoi_t* aoi, location_t* local, region_t* out, uint32_t range) {
	if (local->x - range < 0) {
		out->begin_x = 0;
		out->end_x = 2 * range;
		if (out->end_x >= aoi->tower_x) {
			out->end_x = aoi->tower_x - 1;
		}
	}
	else if (local->x + range >= aoi->tower_x) {
		out->end_x = aoi->tower_x - 1;
		out->begin_x = out->end_x - 2 * range;
		if (out->begin_x < 0) {
			out->begin_x = 0;
		}
	}
	else {
		out->begin_x = local->x - range;
		out->end_x = local->x + range;
	}

	if (local->z - range < 0) {
		out->begin_z = 0;
		out->end_z = 2 * range;
		if (out->end_z >= aoi->tower_z) {
			out->end_z = aoi->tower_z - 1;
		}
	}
	else if (local->z + range >= aoi->tower_z) {
		out->end_z = aoi->tower_z - 1;
		out->begin_z = out->end_z - 2 * range;
	}
	else {
		out->begin_z = local->z - range;
		out->end_z = local->z + range;
		if (out->begin_z < 0) {
			out->begin_z = 0;
		}
	}
}

static inline object_t*
new_object( aoi_t* aoi, int uid, int type, float x, float z) {
	if (aoi->freelist == NULL) {
		return NULL;
	}
	object_t* obj = aoi->freelist;
	aoi->freelist = obj->next;
	obj->next = obj->prev = NULL;
	obj->param.link_node.link_prev = obj->param.link_node.link_next = NULL;
	obj->uid = uid;
	obj->type = type;
	obj->local.x = x;
	obj->local.z = z;
	return obj;
}

static inline void
free_object(aoi_t* aoi, object_t* obj) {
	obj->next = aoi->freelist;
	aoi->freelist = obj;
}

static inline void
link_object(tower_t* tower, object_t* object) {
	if (tower->head == NULL) {
		tower->head = tower->tail = object;
	}
	else {
		tower->tail->next = object;
		object->param.link_node.link_prev = tower->tail;
		object->param.link_node.link_next = NULL;
		tower->tail = object;
	}
}

static inline void
unlink_object(tower_t* tower, object_t* object) {
	if (object->param.link_node.link_prev != NULL) {
		object->param.link_node.link_prev->next = object->param.link_node.link_next;
	}
	else {
		tower->head = object->param.link_node.link_next;
	}
	if (object->param.link_node.link_next != NULL) {
		object->param.link_node.link_next->prev = object->param.link_node.link_prev;
	}
	else {
		tower->tail = object->param.link_node.link_prev;
	}
	object->param.link_node.link_prev = NULL;
	object->param.link_node.link_next = NULL;
}

static inline struct object*
get_object(aoi_t* aoi, int id, int type) {
	if (id < 0 || id >= aoi->max)
		return NULL;

	struct object* result = &aoi->pool[id];
	if (result->type != type)
		return NULL;

	return result;
}

static inline void
check_position(aoi_t* aoi, float x, float z) {
	if (x < 0 || z < 0 || x > aoi->width || z > aoi->height) {
		assert(0);
	}
}

int
create_entity(aoi_t* aoi,int uid,float x,float z,callback_func func,void* ud) {
	check_position(aoi, x, z);

	object_t* entity = new_object(aoi, uid, TYPE_ENTITY, x, z);
	entity->func = func;
	entity->ud = ud;

	location_t out;
	translate(aoi, &entity->local, &out);

	tower_t* tower = &aoi->towers[(uint32_t)out.x][(uint32_t)out.z];

	link_object(tower, entity);

	khiter_t k;
	object_t* other;
	hash_foreach(tower->hash, k, other, {
		if (other->uid != entity->uid) {
			entity->func(entity->uid, other->uid, 1, entity->ud);
		}
	});

	return entity->id;
}

void
remove_entity(aoi_t* aoi,int id) {
	object_t* entity = get_object(aoi, id, TYPE_ENTITY);

	location_t out;
	translate(aoi, &entity->local, &out);

	tower_t* tower = &aoi->towers[(uint32_t)out.x][(uint32_t)out.z];

	unlink_object(tower, entity);

	khiter_t k;
	object_t* other;
	hash_foreach(tower->hash, k, other, {
		if (other->uid != entity->uid) {
			entity->func(entity->uid, other->uid, 0, entity->ud);
		}
	});

	free_object(aoi, entity);
	return;
}

void
move_entity(aoi_t* aoi, int id, float nx, float nz) {
	check_position(aoi, nx, nz);

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

	unlink_object(otower, entity);

	link_object(ntower, entity);

	khiter_t k;
	object_t* leave = NULL;
	object_t* enter = NULL;
	object_t* other;

	hash_foreach(otower->hash, k, other, {
		if (other->uid != entity->uid) {
			if (leave == NULL) {
				leave = other;
				other->next = NULL;
				other->prev = NULL;
			}
			else {
				other->prev = NULL;
				other->next = leave;
				leave = other;
			}
		}
	});

	hash_foreach(ntower->hash, k, other, {
		if (other->uid != entity->uid) {
			if (other->next != NULL || other->prev != NULL || other == leave) {
				if (other->prev != NULL) {
					other->prev->next = other->next;
				}
				if (other->next != NULL) {
					other->next->prev = other->prev;
				}
				if (other == leave) {
					leave = other->next;
				}
				other->next = other->prev = NULL;

			}
			else {
				if (enter == NULL) {
					enter = other;
					other->next = NULL;
					other->prev = NULL;
				}
				else {
					enter->next = enter;
					other->prev = enter;
					other->next = NULL;
					enter = other;
				}
			}
		}
	});

	object_t* cursor = leave;
	while (cursor != NULL) {
		object_t* obj = cursor;
		cursor = cursor->next;
		entity->func(entity->uid, obj->uid, 0, entity->ud);
		obj->next = obj->prev = NULL;
	}

	cursor = enter;
	while (cursor != NULL) {
		object_t* obj = cursor;
		cursor = cursor->next;
		entity->func(entity->uid, obj->uid, 1, entity->ud);
		obj->next = obj->prev = NULL;
	}
}

int
create_trigger(aoi_t* aoi, int uid, float x, float z,int range, callback_func func, void* ud) {
	check_position(aoi, x, z);

	object_t* trigger = new_object(aoi, uid, TYPE_TRIGGER, x, z);
	trigger->param.range = range;
	trigger->func = func;
	trigger->ud = ud;

	location_t out;
	translate(aoi, &trigger->local, &out);

	region_t region;
	get_region(aoi, &out, &region, range);

	int i = 1;

	uint32_t x_index;
	for (x_index = region.begin_x; x_index <= region.end_x; x_index++) {
		uint32_t z_index;
		for (z_index = region.begin_z; z_index <= region.end_z; z_index++) {
			tower_t* tower = &aoi->towers[x_index][z_index];

			hash_set(tower->hash, trigger->uid, trigger);

			struct object* cursor = tower->head;
			while (cursor) {
				if (cursor->uid != trigger->uid) {
					trigger->func(trigger->uid, cursor->uid, 1, trigger->ud);
				}
				cursor = cursor->param.link_node.link_next;
			}
		}
	}
	return trigger->id;
}

void
remove_trigger(aoi_t* aoi,int id) {
	object_t* trigger = get_object(aoi, id, TYPE_TRIGGER);

	location_t out;
	translate(aoi, &trigger->local, &out);

	region_t region;
	get_region(aoi, &out, &region, trigger->param.range);

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

void
move_trigger(aoi_t* aoi, int id, float nx, float nz) {
	check_position( aoi, nx, nz);

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
	get_region(aoi, &oout, &oregion, trigger->param.range);

	region_t nregion;
	get_region(aoi, &nout, &nregion, trigger->param.range);

	uint32_t x_index;
	for (x_index = oregion.begin_x; x_index <= oregion.end_x; x_index++) {
		uint32_t z_index;
		for (z_index = oregion.begin_z; z_index <= oregion.end_z; z_index++) {
			if (x_index >= nregion.begin_x && x_index <= nregion.end_x && z_index >= nregion.begin_z && z_index <= nregion.end_z) {
				continue;
			}
			tower_t* tower = &aoi->towers[x_index][z_index];
			hash_del(tower->hash, trigger->uid);

			struct object* cursor = tower->head;
			while (cursor) {
				if (cursor->uid != trigger->uid) {
					trigger->func(trigger->uid, cursor->uid, 0, trigger->ud);
				}
				cursor = cursor->param.link_node.link_next;
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

			struct object* cursor = tower->head;
			while (cursor) {
				if (cursor->uid != trigger->uid) {
					trigger->func(trigger->uid, cursor->uid, 1, trigger->ud);
				}
				cursor = cursor->param.link_node.link_next;
			}
		}
	}
}

void
release_aoi(aoi_t* aoi) {
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

aoi_t*
create_aoi(int max, int width, int height,int cell) {
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
			tower->head = tower->tail = NULL;
			tower->hash = hash_new();
		}
	}
	return aoi_ctx;
}

void
foreach_aoi_entity(aoi_t* aoi, foreach_entity_func func, void* ud) {
	uint32_t x;
	for (x = 0; x < aoi->tower_x; x++) {
		uint32_t z;
		for (z = 0; z < aoi->tower_z; z++) {
			tower_t* tower = &aoi->towers[x][z];
			object_t* cursor = tower->head;
			while (cursor != NULL) {
				object_t* entity = cursor;
				func(entity->uid, entity->local.x, entity->local.z, ud);
				cursor = cursor->next;
			}
		}
	}
}

void
foreach_aoi_trigger(aoi_t* aoi, foreach_trigger_func func, void* ud) {
	hash_t* hash = hash_new();

	uint32_t x;
	for (x = 0; x < aoi->tower_x; x++) {
		uint32_t z;
		for (z = 0; z < aoi->tower_z; z++) {
			tower_t* tower = &aoi->towers[x][z];
			khiter_t k;
			object_t* trigger;
			hash_foreach(tower->hash, k, trigger, {
				if (!hash_has(hash,trigger->uid)) {
					hash_set(hash, trigger->uid, trigger);
				}
			});
		}
	}

	khiter_t k;
	object_t* trigger;
	hash_foreach(hash, k, trigger, {
		func(trigger->uid, trigger->local.x, trigger->local.z, trigger->param.range, ud);
	});
	hash_free(hash);
}