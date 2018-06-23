#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "aoi.h"
#include "pool.h"
#include "object_container.h"

#ifdef _WIN32
#define inline __inline
#endif

#define MAX_LEVEL 8
#define COMMON_LEVEL 0

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
	int level;
} object_t;

typedef struct link_list {
	object_t* head;
	object_t* tail;
} link_list_t;

typedef struct tile {
	link_list_t **headers;
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

static inline struct tile*
tile_withrc(struct aoi_context* ctx, int r, int c) {
	if ( c > ctx->max_x_index || r > ctx->max_z_index )
		return NULL;
	return &ctx->tiles[r * ( ctx->max_x_index + 1 ) + c];
}

static inline void
tile_init(struct aoi_context* ctx) {
	ctx->tiles = malloc(ctx->tile_size * sizeof( struct tile ));
	int x, z;
	for ( x = 0; x <= ctx->max_x_index; x++ ) {
		for ( z = 0; z <= ctx->max_z_index; z++ ) {
			struct tile* tl = tile_withrc(ctx, z, x);
			tl->x = x;
			tl->z = z;
			tl->headers = malloc(MAX_LEVEL * sizeof( struct list * ));
			memset(tl->headers, 0, MAX_LEVEL * sizeof( struct list * ));
		}
	}
}

static inline struct tile*
tile_withpos(struct aoi_context* ctx, struct location *pos) {
	int x = pos->x / ctx->tile_cell;
	int z = pos->z / ctx->tile_cell;
	if ( x > ctx->max_x_index || z > ctx->max_z_index )
		return NULL;
	return tile_withrc(ctx, z, x);
}

static inline link_list_t*
tile_level(struct tile* tl, int level) {
	if ( tl->headers[level] == NULL ) {
		tl->headers[level] = malloc(sizeof( link_list_t ));
		memset(tl->headers[level], 0, sizeof( link_list_t ));
		tl->headers[level]->head = tl->headers[level]->tail = NULL;
	}
	return tl->headers[level];
}

static inline void
tile_push(struct tile* tl, int level, object_t* object) {
	link_list_t* list = tile_level(tl, level);

	if ( list->head == NULL ) {
		assert(list->head == list->tail);
		list->head = list->tail = object;
	}
	else {
		if ( list->head == list->tail ) {
			list->head = object;
			object->next = list->tail;
			list->tail->prev = object;
		}
		else {
			object->next = list->head;
			list->head->prev = object;
			list->head = object;
		}
	}
}

static inline void
tile_pop(struct tile* tl, int level, object_t* object) {
	link_list_t* list = tile_level(tl, level);

	if ( list->head == list->tail ) {
		assert(object == list->head);
		list->head = list->tail = NULL;
	}
	else {
		if ( object->prev ) {
			object->prev->next = object->next;
		}
		if ( object->next ) {
			object->next->prev = object->prev;
		}

		if ( object == list->head ) {
			list->head = object->next;
		}
		else if ( object == list->tail ) {
			list->tail = object->prev;
		}
	}
}

static inline void
assert_pos(aoi_context_t* ctx, float x, float z) {
	if ( x < 0 || z < 0 || x >= ctx->width || z >= ctx->height ) {
		fprintf(stderr, "invalide pos[%f:%f]", x, z);
		assert(0);
	}
}

static inline int
get_region(aoi_context_t* ctx, location_t *pos, int range, location_t *bl, location_t *tr) {
	tile_t *tl = tile_withpos(ctx, pos);
	if ( tl == NULL )
		return -1;

	bl->x = tl->x - range;
	bl->z = tl->z - range;
	tr->x = tl->x + range;
	tr->z = tl->z + range;

	if ( bl->x < 0 )
		bl->x = 0;
	if ( bl->z < 0 )
		bl->z = 0;
	if ( tr->x > ctx->max_x_index )
		tr->x = ctx->max_x_index;
	if ( tr->z > ctx->max_z_index )
		tr->z = ctx->max_z_index;

	return 0;
}

void
forearch_list(link_list_t *list, object_t *self, callback_func func, void* ud) {
	struct object *cursor = list->head;
	while ( cursor ) {
		if ( cursor == self ) {
			cursor = cursor->next;
			continue;
		}
		func(self->uid, cursor->uid, ud);
		cursor = cursor->next;
	}
}

aoi_context_t*
aoi_create(int width, int height, int cell, int range, int max, callback_func enter_func, callback_func leave_func) {
	int max_x_index = width / cell - 1;
	int max_z_index = height / cell - 1;

	width = ( max_x_index + 1 ) * cell;
	height = ( max_z_index + 1 ) * cell;

	aoi_context_t* ctx = malloc(sizeof( *ctx ));
	memset(ctx, 0, sizeof( *ctx ));

	ctx->pool = pool_create(sizeof( object_t ));
	ctx->container = container_create(max);

	ctx->width = width;
	ctx->height = height;
	ctx->max_x_index = max_x_index;
	ctx->max_z_index = max_z_index;
	ctx->tile_cell = cell;
	ctx->tile_size = ( max_x_index + 1 ) * ( max_z_index + 1 );
	ctx->range = range;
	ctx->enter_func = enter_func;
	ctx->leave_func = leave_func;

	tile_init(ctx);

	return ctx;
}

void
aoi_release(aoi_context_t* ctx) {
	pool_release(ctx->pool);
	container_release(ctx->container);
	int x, z;
	for ( x = 0; x <= ctx->max_x_index; x++ ) {
		for ( z = 0; z <= ctx->max_z_index; z++ ) {
			struct tile* tl = tile_withrc(ctx, z, x);
			int i;
			for ( i = 0; i < MAX_LEVEL; i++ ) {
				if ( tl->headers[i] )
					free(tl->headers[i]);
			}
			free(tl->headers);
		}
	}

	free(ctx->tiles);
}

int
aoi_enter(aoi_context_t* ctx, int uid, float x, float z, int level, void* ud) {
	assert_pos(ctx, x, z);
	if ( level < 0 || level  > MAX_LEVEL ) {
		return -1;
	}

	object_t* self = pool_malloc(ctx->pool);
	memset(self, 0, sizeof( *self ));

	int id = container_add(ctx->container, self);
	if ( id < 0 ) {
		pool_free(ctx->pool, self);
		return -1;
	}

	self->id = id;
	self->uid = uid;
	self->locat.x = x;
	self->locat.z = z;
	self->level = level;

	tile_t *tl = tile_withpos(ctx, &self->locat);
	assert(tl != NULL);

	struct location bl = { 0, 0 };
	struct location tr = { 0, 0 };
	get_region(ctx, &self->locat, ctx->range, &bl, &tr);

	for ( z = bl.z; z <= tr.z; z++ ) {
		for ( x = bl.x; x <= tr.x; x++ ) {
			tile_t *tl = tile_withrc(ctx, z, x);
			link_list_t *list = tile_level(tl, COMMON_LEVEL);
			forearch_list(list, self, ctx->enter_func, ud);

			if ( self->level != COMMON_LEVEL ) {
				list = tile_level(tl, self->level);
				forearch_list(list, self, ctx->enter_func, ud);
			}
		}
	}
	tile_push(tl, self->level, self);
	return id;
}

int
aoi_leave(aoi_context_t* ctx, int id, void* ud) {
	object_t* self = container_get(ctx->container, id);
	if ( !self ) {
		return -1;
	}

	struct tile *tl = tile_withpos(ctx, &self->locat);
	assert(tl != NULL);

	struct location bl = { 0, 0 };
	struct location tr = { 0, 0 };
	get_region(ctx, &self->locat, ctx->range, &bl, &tr);
	

	int array_index = 1;
	int x, z;
	for ( z = bl.z; z <= tr.z; z++ ) {
		for ( x = bl.x; x <= tr.x; x++ ) {
			struct tile *tl = tile_withrc(ctx, z, x);
			if ( tl == NULL ) {
				return -1;
			}
				
			link_list_t *list = tile_level(tl, COMMON_LEVEL);
			forearch_list(list, self, ctx->leave_func, ud);

			if ( self->level != COMMON_LEVEL ) {
				link_list_t *list = tile_level(tl, self->level);
				forearch_list(list, self, ctx->leave_func, ud);
			}
		}
	}

	tile_pop(tl, self->level, self);

	container_remove(ctx->container, id);

	pool_free(ctx->pool, self);

	return 1;
}

int
aoi_update(aoi_context_t* ctx, int id, float x, float z, void* ud) {
	object_t* self = container_get(ctx->container, id);
	if ( !self ) {
		return -1;
	}

	assert_pos(ctx, x, z);

	struct location op = self->locat;
	self->locat.x = x;
	self->locat.z = z;

	struct tile *otl = tile_withpos(ctx, &op);
	struct tile *ntl = tile_withpos(ctx, &self->locat);
	if ( otl == ntl )
		return 0;

	tile_pop(otl, self->level, self);
	tile_push(ntl, self->level, self);

	struct location obl, otr;
	get_region(ctx, &op, ctx->range, &obl, &otr);
	
	struct location nbl, ntr;
	get_region(ctx, &self->locat, ctx->range, &nbl, &ntr);

	int array_index = 1;

	for ( z = nbl.z; z <= ntr.z; z++ ) {
		for ( x = nbl.x; x <= ntr.x; x++ ) {
			if ( x >= obl.x && x <= otr.x && z >= obl.z && z <= otr.z )
				continue;

			struct tile *tl = tile_withrc(ctx, z, x);
			if ( tl == NULL ) {
				return -1;
			}

			link_list_t *list = tile_level(tl, COMMON_LEVEL);
			forearch_list(list, self, ctx->enter_func, ud);

			if ( self->level != COMMON_LEVEL ) {
				link_list_t *list = tile_level(tl, self->level);
				forearch_list(list, self, ctx->enter_func, ud);
			}
		}
	}

	for ( z = obl.z; z <= otr.z; z++ ) {
		for ( x = obl.x; x <= otr.x; x++ ) {
			if ( x >= nbl.x && x <= ntr.x && z >= nbl.z && z <= ntr.z )
				continue;

			struct tile *tl = tile_withrc(ctx, z, x);
			if ( tl == NULL ) {
				return -1;
			}
				
			link_list_t *list = tile_level(tl, COMMON_LEVEL);
			forearch_list(list, self, ctx->leave_func, ud);

			if ( self->level != COMMON_LEVEL ) {
				link_list_t *list = tile_level(tl, self->level);
				forearch_list(list, self, ctx->leave_func, ud);
			}
		}
	}

	return 0;
}

int
get_viewlist(aoi_context_t* ctx,int id, callback_func func,void* ud) {
	object_t* self = container_get(ctx->container, id);

	struct tile *tl = tile_withpos(ctx, &self->locat);
	assert(tl != NULL);

	struct location bl = { 0, 0 };
	struct location tr = { 0, 0 };
	get_region(ctx, &self->locat, ctx->range, &bl, &tr);

	int x, z;
	for ( z = bl.z; z <= tr.z; z++ ) {
		for ( x = bl.x; x <= tr.x; x++ ) {
			struct tile *tl = tile_withrc(ctx, z, x);
			assert(tl != NULL);

			link_list_t *list = tile_level(tl, COMMON_LEVEL);
			forearch_list(list, self, func, ud);

			if ( self->level != COMMON_LEVEL ) {
				link_list_t *list = tile_level(tl, self->level);
				forearch_list(list, self, func, ud);
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


