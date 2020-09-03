#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "simple-aoi.h"
#include "pool.h"
#include "object_container.h"

#ifdef _WIN32
#define inline __inline
#endif

#define FLAG_SELF		0x1
#define FLAG_OTHER		0x2


typedef struct location {
	float x;
	float z;
} location_t;

typedef struct object {
	struct object* prev;
	struct object* next;
	location_t locat;
	int id;
	int uid;
	int layer;
} object_t;

typedef struct tile {
	object_t *headers;
	int x;
	int z;
} tile_t;

typedef struct aoi_context {
	int width;
	int height;
	int max_x_index;
	int max_z_index;
	int range;
	int tile_cell;
	int tile_size;

	callback_func enter_func;
	callback_func leave_func;

	struct pool_ctx* pool;
	struct object_container* container;

	struct tile *tiles;
} aoi_context_t;

static const char* ERROR_MESSAGE[] = {
	"ok",
	"error pos",
	"error layer",
	"error object id"
};

static inline tile_t* tile_withrc(struct aoi_context* ctx, int r, int c);

static inline void tile_init(struct aoi_context* ctx) {
	ctx->tiles = malloc(ctx->tile_size * sizeof(struct tile));
	int x, z;
	for (x = 0; x <= ctx->max_x_index; x++) {
		for (z = 0; z <= ctx->max_z_index; z++) {
			struct tile* tl = tile_withrc(ctx, z, x);
			tl->x = x;
			tl->z = z;
			tl->headers = malloc(LAYER_MAX * sizeof(object_t));
			memset(tl->headers, 0, LAYER_MAX * sizeof(object_t));
			int i;
			for (i = 0; i < LAYER_MAX; i++) {
				object_t* header = &tl->headers[i];
				header->prev = header->next = header;
			}
		}
	}
}

static inline tile_t* tile_withrc(struct aoi_context* ctx, int r, int c) {
	if (c > ctx->max_x_index || r > ctx->max_z_index)
		return NULL;
	return &ctx->tiles[r * (ctx->max_x_index + 1) + c];
}

static inline tile_t* tile_withpos(struct aoi_context* ctx, struct location *pos) {
	int x = pos->x / ctx->tile_cell;
	int z = pos->z / ctx->tile_cell;
	if (x > ctx->max_x_index || z > ctx->max_z_index)
		return NULL;
	return tile_withrc(ctx, z, x);
}

static inline object_t* tile_layer(struct tile* tl, int layer) {
	return &tl->headers[layer];
}

static inline void tile_push(struct tile* tl, int layer, object_t* object) {
	object_t* header = tile_layer(tl, layer);
	object_t* prev = header->prev;
	prev->next = object;
	object->next = header;
	object->prev = prev;
	header->prev = object;
}

static inline void tile_pop(struct tile* tl, int layer, object_t* object) {
	object_t* next = object->next;
	object_t* prev = object->prev;
	next->prev = prev;
	prev->next = next;
	object->prev = NULL;
	object->next = NULL;
}

static inline int get_region(aoi_context_t* ctx, location_t *pos, int range, location_t *bl, location_t *tr) {
	tile_t *tl = tile_withpos(ctx, pos);
	if (tl == NULL)
		return -1;

	bl->x = tl->x - range;
	bl->z = tl->z - range;
	tr->x = tl->x + range;
	tr->z = tl->z + range;

	if (bl->x < 0)
		bl->x = 0;
	if (bl->z < 0)
		bl->z = 0;
	if (tr->x > ctx->max_x_index)
		tr->x = ctx->max_x_index;
	if (tr->z > ctx->max_z_index)
		tr->z = ctx->max_z_index;

	return 0;
}

static inline void forearch_list(object_t *header, object_t *self, int flag, callback_func func, void* ud) {
	struct object *cursor = header->next;
	while (cursor != header) {
		if (cursor == self) {
			cursor = cursor->next;
			continue;
		}
		if (flag & FLAG_SELF) {
			func(self->uid, cursor->uid, ud);
		}
		if (flag & FLAG_OTHER) {
			func(cursor->uid, self->uid, ud);
		}
		cursor = cursor->next;
	}
}

aoi_context_t* aoi_create(int width, int height, int cell, int range, int max, callback_func enter_func, callback_func leave_func) {
	int max_x_index = width / cell;
	int max_z_index = height / cell;

	aoi_context_t* ctx = malloc(sizeof(*ctx));
	memset(ctx, 0, sizeof(*ctx));

	ctx->pool = pool_create(sizeof(object_t));
	ctx->container = container_create(max);

	ctx->width = width;
	ctx->height = height;
	ctx->max_x_index = max_x_index;
	ctx->max_z_index = max_z_index;
	ctx->tile_cell = cell;
	ctx->tile_size = (max_x_index + 1) * (max_z_index + 1);
	ctx->range = range;
	ctx->enter_func = enter_func;
	ctx->leave_func = leave_func;

	tile_init(ctx);

	return ctx;
}

void aoi_release(aoi_context_t* ctx) {
	pool_release(ctx->pool);
	container_release(ctx->container);
	int x, z;
	for (x = 0; x <= ctx->max_x_index; x++) {
		for (z = 0; z <= ctx->max_z_index; z++) {
			struct tile* tl = tile_withrc(ctx, z, x);
			free(tl->headers);
		}
	}

	free(ctx->tiles);
}

int aoi_enter(aoi_context_t* ctx, int uid, float x, float z, int layer, void* ud) {
	if (x < 0 || z < 0 || x > ctx->width || z > ctx->height) {
		return ERROR_POS;
	}
	if (layer < 0 || layer >= LAYER_MAX) {
		return ERROR_LAYER;
	}

	object_t* self = pool_malloc(ctx->pool);
	memset(self, 0, sizeof(*self));

	int id = container_add(ctx->container, self);

	self->id = id;
	self->uid = uid;
	self->locat.x = x;
	self->locat.z = z;
	self->layer = layer;

	tile_t *tl = tile_withpos(ctx, &self->locat);
	assert(tl != NULL);

	struct location bl = { 0, 0 };
	struct location tr = { 0, 0 };
	get_region(ctx, &self->locat, ctx->range, &bl, &tr);

	for (z = bl.z; z <= tr.z; z++) {
		for (x = bl.x; x <= tr.x; x++) {
			tile_t *tl = tile_withrc(ctx, z, x);

			if (self->layer == LAYER_ITEM) {
				object_t* header = tile_layer(tl, LAYER_USER);
				forearch_list(header, self, FLAG_OTHER, ctx->enter_func, ud);
			} else if (self->layer == LAYER_MONSTER) {
				object_t* header = tile_layer(tl, LAYER_USER);
				forearch_list(header, self, FLAG_OTHER | FLAG_SELF, ctx->enter_func, ud);
			} else {
				object_t* header = tile_layer(tl, LAYER_ITEM);
				forearch_list(header, self, FLAG_SELF, ctx->enter_func, ud);

				header = tile_layer(tl, LAYER_MONSTER);
				forearch_list(header, self, FLAG_OTHER | FLAG_SELF, ctx->enter_func, ud);

				header = tile_layer(tl, LAYER_USER);
				forearch_list(header, self, FLAG_OTHER | FLAG_SELF, ctx->enter_func, ud);
			}
		}
	}
	tile_push(tl, self->layer, self);
	return id;
}

int aoi_leave(aoi_context_t* ctx, int id, void* ud) {
	object_t* self = container_get(ctx->container, id);
	if (!self) {
		return ERROR_OBJECT_ID;
	}

	struct tile *tl = tile_withpos(ctx, &self->locat);

	struct location bl = { 0, 0 };
	struct location tr = { 0, 0 };
	get_region(ctx, &self->locat, ctx->range, &bl, &tr);

	int x, z;
	for (z = bl.z; z <= tr.z; z++) {
		for (x = bl.x; x <= tr.x; x++) {
			struct tile *tl = tile_withrc(ctx, z, x);
			if (tl == NULL) {
				return -1;
			}

			if (self->layer == LAYER_ITEM || self->layer == LAYER_MONSTER) {
				object_t *header = tile_layer(tl, LAYER_USER);
				forearch_list(header, self, FLAG_OTHER, ctx->leave_func, ud);
			} else {
				object_t *header = tile_layer(tl, LAYER_MONSTER);
				forearch_list(header, self, FLAG_OTHER, ctx->enter_func, ud);

				header = tile_layer(tl, LAYER_USER);
				forearch_list(header, self, FLAG_OTHER, ctx->enter_func, ud);
			}
		}
	}

	tile_pop(tl, self->layer, self);

	container_remove(ctx->container, id);

	pool_free(ctx->pool, self);

	return 0;
}

int aoi_update(aoi_context_t* ctx, int id, float x, float z, void* ud) {
	object_t* self = container_get(ctx->container, id);
	if (!self) {
		return ERROR_OBJECT_ID;
	}

	if (x < 0 || z < 0 || x > ctx->width || z > ctx->height) {
		return ERROR_POS;
	}

	struct location op = self->locat;
	self->locat.x = x;
	self->locat.z = z;

	struct tile *otl = tile_withpos(ctx, &op);
	struct tile *ntl = tile_withpos(ctx, &self->locat);
	if (otl == ntl)
		return 0;

	tile_pop(otl, self->layer, self);
	tile_push(ntl, self->layer, self);

	struct location obl = { 0 };
	struct location otr = { 0 };
	get_region(ctx, &op, ctx->range, &obl, &otr);

	struct location nbl = { 0 };
	struct location ntr = { 0 };
	get_region(ctx, &self->locat, ctx->range, &nbl, &ntr);

	for (z = nbl.z; z <= ntr.z; z++) {
		for (x = nbl.x; x <= ntr.x; x++) {
			if (x >= obl.x && x <= otr.x && z >= obl.z && z <= otr.z)
				continue;

			struct tile *tl = tile_withrc(ctx, z, x);

			if (self->layer == LAYER_ITEM) {
				object_t *header = tile_layer(tl, LAYER_USER);
				forearch_list(header, self, FLAG_OTHER, ctx->enter_func, ud);
			} else if (self->layer == LAYER_MONSTER) {
				object_t *header = tile_layer(tl, LAYER_USER);
				forearch_list(header, self, FLAG_OTHER | FLAG_SELF, ctx->enter_func, ud);
			} else {
				object_t *header = tile_layer(tl, LAYER_ITEM);
				forearch_list(header, self, FLAG_SELF, ctx->enter_func, ud);

				header = tile_layer(tl, LAYER_MONSTER);
				forearch_list(header, self, FLAG_OTHER | FLAG_SELF, ctx->enter_func, ud);

				header = tile_layer(tl, LAYER_USER);
				forearch_list(header, self, FLAG_OTHER | FLAG_SELF, ctx->enter_func, ud);
			}
		}
	}

	for (z = obl.z; z <= otr.z; z++) {
		for (x = obl.x; x <= otr.x; x++) {
			if (x >= nbl.x && x <= ntr.x && z >= nbl.z && z <= ntr.z)
				continue;

			struct tile *tl = tile_withrc(ctx, z, x);

			if (self->layer == LAYER_ITEM) {
				object_t *header = tile_layer(tl, LAYER_USER);
				forearch_list(header, self, FLAG_OTHER, ctx->leave_func, ud);
			} else if (self->layer == LAYER_MONSTER) {
				object_t *header = tile_layer(tl, LAYER_USER);
				forearch_list(header, self, FLAG_OTHER | FLAG_SELF, ctx->leave_func, ud);
			} else {
				object_t *header = tile_layer(tl, LAYER_ITEM);
				forearch_list(header, self, FLAG_SELF, ctx->leave_func, ud);

				header = tile_layer(tl, LAYER_MONSTER);
				forearch_list(header, self, FLAG_OTHER | FLAG_SELF, ctx->leave_func, ud);

				header = tile_layer(tl, LAYER_USER);
				forearch_list(header, self, FLAG_OTHER | FLAG_SELF, ctx->leave_func, ud);
			}
		}
	}

	return 0;
}

int aoi_uid(aoi_context_t* ctx, int id) {
	object_t* self = container_get(ctx->container, id);
	if (!self) {
		return ERROR_OBJECT_ID;
	}
	return self->uid;
}

const char* aoi_error(int no) {
	return ERROR_MESSAGE[-no];
}

int get_witness(aoi_context_t* ctx, int id, callback_func func, void* ud) {
	object_t* self = container_get(ctx->container, id);

	struct tile *tl = tile_withpos(ctx, &self->locat);
	assert(tl != NULL);

	struct location bl = { 0, 0 };
	struct location tr = { 0, 0 };
	get_region(ctx, &self->locat, ctx->range, &bl, &tr);

	int x, z;
	for (z = bl.z; z <= tr.z; z++) {
		for (x = bl.x; x <= tr.x; x++) {
			struct tile *tl = tile_withrc(ctx, z, x);
			assert(tl != NULL);

			if (self->layer == LAYER_ITEM) {
				object_t *header = tile_layer(tl, LAYER_USER);
				forearch_list(header, self, FLAG_OTHER, func, ud);
			} else if (self->layer == LAYER_MONSTER) {
				object_t *header = tile_layer(tl, LAYER_USER);
				forearch_list(header, self, FLAG_OTHER, func, ud);
			} else {
				object_t *header = tile_layer(tl, LAYER_MONSTER);
				forearch_list(header, self, FLAG_OTHER, func, ud);

				header = tile_layer(tl, LAYER_USER);
				forearch_list(header, self, FLAG_OTHER, func, ud);
			}
		}
	}

	return 0;
}

int get_visible(aoi_context_t* ctx, int id, callback_func func, void* ud) {
	object_t* self = container_get(ctx->container, id);
	if (self->layer == LAYER_ITEM) {
		return 0;
	}
	struct tile *tl = tile_withpos(ctx, &self->locat);
	assert(tl != NULL);

	struct location bl = { 0, 0 };
	struct location tr = { 0, 0 };
	get_region(ctx, &self->locat, ctx->range, &bl, &tr);

	int x, z;
	for (z = bl.z; z <= tr.z; z++) {
		for (x = bl.x; x <= tr.x; x++) {
			struct tile *tl = tile_withrc(ctx, z, x);
			assert(tl != NULL);

			if (self->layer == LAYER_MONSTER) {
				object_t *header = tile_layer(tl, LAYER_USER);
				forearch_list(header, self, FLAG_OTHER, func, ud);
			} else {
				object_t *header = tile_layer(tl, LAYER_MONSTER);
				forearch_list(header, self, FLAG_OTHER, func, ud);

				header = tile_layer(tl, LAYER_USER);
				forearch_list(header, self, FLAG_OTHER, func, ud);
			}
		}
	}

	return 0;
}

struct foreach_param {
	forearch_func func;
	void* ud;
};

void
foreach_callback(int id, object_t* object, void* ud) {
	struct foreach_param* param = ud;
	param->func(object->uid, object->locat.x, object->locat.z, param->ud);
}

void
forearch_object(aoi_context_t* ctx, forearch_func func, void* ud) {
	struct foreach_param param;
	param.func = func;
	param.ud = ud;
	container_foreach(ctx->container, foreach_callback, &param);
}


