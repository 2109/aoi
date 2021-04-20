#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "link-aoi.h"
#include "hash_witness.h"

#define AOI_ENTITY 		1
#define AOI_LOW_BOUND 	2
#define AOI_HIGH_BOUND 	4

#define UNLIMITED -1000

#define IN -1
#define OUT 1

//#define LINKAOI_HAVE_RESTORE_WITNESS
//#define LINKAOI_HAVE_RESTORE_VISIBLE

#ifdef _WIN32
#define inline __inline
#endif

struct aoi_entity;
struct aoi_trigger;
struct aoi_context;
struct linknode;
struct position;

typedef int(*cmp_func)(struct position*, struct position*);
typedef void(*shuffle_func)(struct aoi_context*, struct linknode*, int);

typedef struct position {
	int x;
	int z;
} position_t;

typedef struct aoi_object {
	struct aoi_object* next;
	struct aoi_object* prev;
	int uid;

	struct aoi_entity* entity;
	struct aoi_trigger* trigger;
	callback_func enter_func;
	callback_func leave_func;
	void* ud;

	int inout;
} aoi_object_t;

typedef struct linknode {
	struct linknode* prev;
	struct linknode* next;
	aoi_object_t* owner;
	position_t pos;
	shuffle_func shuffle;
	uint8_t flag;
	int8_t order;
} linknode_t;

typedef struct aoi_entity {
	position_t center;
	position_t ocenter;
	linknode_t node[2];
#ifdef LINKAOI_HAVE_RESTORE_WITNESS
	hash_set_t* witness;
#endif
} aoi_entity_t;

typedef struct aoi_trigger {
	position_t center;
	position_t ocenter;
	linknode_t node[4];
	int range;
#ifdef LINKAOI_HAVE_RESTORE_VISIBLE
	hash_set_t* visible;
#endif
} aoi_trigger_t;

typedef struct aoi_context {
	linknode_t linklist[2];

	aoi_object_t* freelist;

	struct aoi_object enter_list;
	struct aoi_object leave_list;
} aoi_context_t;

static inline int dt_x_range(position_t* entity, position_t* trigger) {
	return abs(entity->x - trigger->x);
}

static inline int dt_z_range(position_t* entity, position_t* trigger) {
	return abs(entity->z - trigger->z);
}

static inline void insert_node(aoi_context_t* ctx, int axis_x, linknode_t* node) {
	linknode_t* link;
	if (axis_x) {
		link = &ctx->linklist[0];
	} else {
		link = &ctx->linklist[1];
	}

	linknode_t* next = link->next;
	next->prev = node;
	node->next = next;
	node->prev = link;
	link->next = node;
}

static inline void remove_node(linknode_t* node) {
	linknode_t* next = node->next;
	linknode_t* prev = node->prev;
	next->prev = prev;
	prev->next = next;
	node->prev = NULL;
	node->next = NULL;
}

static inline void exchange_node(linknode_t* lhs, linknode_t* rhs) {
	remove_node(lhs);
	linknode_t* next = rhs->next;
	rhs->next = lhs;
	lhs->prev = rhs;
	lhs->next = next;
	next->prev = lhs;
}

static inline void link_enter_list(aoi_context_t* ctx, aoi_object_t* self, aoi_object_t* other, cmp_func cmp, int is_entity) {
	if (other->inout == IN || other->inout == OUT) {
		return;
	}

	if (other->inout == 0) {
		if (is_entity) {
			int in = cmp(&self->entity->center, &other->trigger->center) <= other->trigger->range;
			if (!in)
				return;
		} else {
			int in = cmp(&other->entity->center, &self->trigger->center) <= self->trigger->range;
			if (!in)
				return;
		}
	}

	aoi_object_t* prev = ctx->enter_list.prev;
	prev->next = other;
	other->next = &ctx->enter_list;
	other->prev = prev;
	ctx->enter_list.prev = other;

	other->inout = IN;
}

static inline void link_leave_list(aoi_context_t* ctx, aoi_object_t* self, aoi_object_t* other, cmp_func cmp, int is_entity) {
	if (other->inout == OUT) {
		return;
	}

	if (other->inout == IN) {
		aoi_object_t* next = other->next;
		aoi_object_t* prev = other->prev;
		next->prev = prev;
		prev->next = next;

		other->next = other->prev = NULL;
		other->inout = 0;
		return;
	} else {
		if (is_entity) {
			if (cmp(&self->entity->ocenter, &other->trigger->center) > other->trigger->range)
				return;
		} else {
			if (cmp(&other->entity->center, &self->trigger->ocenter) > self->trigger->range) {
				return;
			}
		}
	}

	aoi_object_t* prev = ctx->leave_list.prev;
	prev->next = other;
	other->next = &ctx->leave_list;
	other->prev = prev;
	ctx->leave_list.prev = other;

	other->inout = OUT;
}

static void entity_shuffle_x(aoi_context_t* ctx, linknode_t* node, int x) {
	node->pos.x = x;
	linknode_t* link = &ctx->linklist[0];
	if (link->next == link) {
		return;
	}

	while (node->prev != link && ((node->pos.x < node->prev->pos.x) || (node->pos.x == node->prev->pos.x && node->order <= node->prev->order))) {
		linknode_t* other = node->prev;
		exchange_node(node->prev, node);

		if (other->flag & AOI_LOW_BOUND) {
			link_leave_list(ctx, node->owner, other->owner, dt_z_range, 1);
		} else if (other->flag & AOI_HIGH_BOUND) {
			link_enter_list(ctx, node->owner, other->owner, dt_z_range, 1);
		}
	}

	while (node->next != link && ((node->pos.x > node->next->pos.x) || (node->pos.x == node->next->pos.x && node->order >= node->next->order))) {
		linknode_t* other = node->next;
		exchange_node(node, node->next);

		if (other->flag & AOI_LOW_BOUND) {
			link_enter_list(ctx, node->owner, other->owner, dt_z_range, 1);
		} else if (other->flag & AOI_HIGH_BOUND) {
			link_leave_list(ctx, node->owner, other->owner, dt_z_range, 1);
		}
	}
}

static void entity_shuffle_z(aoi_context_t* ctx, linknode_t* node, int z) {
	node->pos.z = z;
	linknode_t* link = &ctx->linklist[1];
	if (link->next == link) {
		return;
	}
	while (node->prev != link && ((node->pos.z < node->prev->pos.z) || (node->pos.z == node->prev->pos.z && node->order <= node->prev->order))) {
		linknode_t* other = node->prev;
		exchange_node(node->prev, node);

		if (other->flag & AOI_LOW_BOUND) {
			link_leave_list(ctx, node->owner, other->owner, dt_x_range, 1);
		} else if (other->flag & AOI_HIGH_BOUND) {
			link_enter_list(ctx, node->owner, other->owner, dt_x_range, 1);
		}
	}

	while (node->next != link && ((node->pos.z > node->next->pos.z) || (node->pos.z == node->next->pos.z && node->order >= node->next->order))) {
		linknode_t* other = node->next;
		exchange_node(node, node->next);

		if (other->flag & AOI_LOW_BOUND) {
			link_enter_list(ctx, node->owner, other->owner, dt_x_range, 1);
		} else if (other->flag & AOI_HIGH_BOUND) {
			link_leave_list(ctx, node->owner, other->owner, dt_x_range, 1);
		}
	}
}

static void trigger_low_bound_shuffle_x(aoi_context_t* ctx, linknode_t* node, int x) {
	node->pos.x = x;
	linknode_t* link = &ctx->linklist[0];
	if (link->next == link) {
		return;
	}

	while (node->prev != link && ((node->pos.x < node->prev->pos.x) || (node->pos.x == node->prev->pos.x && node->order <= node->prev->order))) {
		linknode_t* other = node->prev;
		exchange_node(node->prev, node);
		if (other->flag & AOI_ENTITY) {
			link_enter_list(ctx, node->owner, other->owner, dt_z_range, 0);
		}
	}

	while (node->next != link && ((node->pos.x > node->next->pos.x) || (node->pos.x == node->next->pos.x && node->order >= node->next->order))) {
		linknode_t* other = node->next;
		exchange_node(node, node->next);
		if (other->flag & AOI_ENTITY) {
			link_leave_list(ctx, node->owner, other->owner, dt_z_range, 0);
		}
	}
}

static void trigger_low_bound_shuffle_z(aoi_context_t* ctx, linknode_t* node, int z) {
	node->pos.z = z;
	linknode_t* link = &ctx->linklist[1];
	if (link->next == link) {
		return;
	}

	while (node->prev != link && ((node->pos.z < node->prev->pos.z) || (node->pos.z == node->prev->pos.z && node->order <= node->prev->order))) {
		linknode_t* other = node->prev;
		exchange_node(node->prev, node);
		if (other->flag & AOI_ENTITY) {
			link_enter_list(ctx, node->owner, other->owner, dt_x_range, 0);
		}
	}

	while (node->next != link && ((node->pos.z > node->next->pos.z) || (node->pos.z == node->next->pos.z && node->order >= node->next->order))) {
		linknode_t* other = node->next;
		exchange_node(node, node->next);
		if (other->flag & AOI_ENTITY) {
			link_leave_list(ctx, node->owner, other->owner, dt_x_range, 0);
		}
	}
}

static void trigger_high_bound_shuffle_x(aoi_context_t* ctx, linknode_t* node, int x) {
	node->pos.x = x;
	linknode_t* link = &ctx->linklist[0];
	if (link->next == link) {
		return;
	}

	while (node->prev != link && ((node->pos.x < node->prev->pos.x) || (node->pos.x == node->prev->pos.x && node->order <= node->prev->order))) {
		linknode_t* other = node->prev;
		exchange_node(node->prev, node);
		if (other->flag & AOI_ENTITY) {
			link_leave_list(ctx, node->owner, other->owner, dt_z_range, 0);
		}
	}

	while (node->next != link && ((node->pos.x > node->next->pos.x) || (node->pos.x == node->next->pos.x && node->order >= node->next->order))) {
		linknode_t* other = node->next;
		exchange_node(node, node->next);
		if (other->flag & AOI_ENTITY) {
			link_enter_list(ctx, node->owner, other->owner, dt_z_range, 0);
		}
	}
}

static void trigger_high_bound_shuffle_z(aoi_context_t* ctx, linknode_t* node, int z) {
	node->pos.z = z;
	linknode_t* link = &ctx->linklist[1];
	if (link->next == link) {
		return;
	}

	while (node->prev != link && ((node->pos.z < node->prev->pos.z) || (node->pos.z == node->prev->pos.z && node->order <= node->prev->order))) {
		linknode_t* other = node->prev;
		exchange_node(node->prev, node);
		if (other->flag & AOI_ENTITY) {
			link_leave_list(ctx, node->owner, other->owner, dt_x_range, 0);
		}
	}

	while (node->next != link && ((node->pos.z > node->next->pos.z) || (node->pos.z == node->next->pos.z && node->order >= node->next->order))) {
		linknode_t* other = node->next;
		exchange_node(node, node->next);
		if (other->flag & AOI_ENTITY) {
			link_enter_list(ctx, node->owner, other->owner, dt_x_range, 0);
		}
	}
}

static void shuffle_entity(aoi_context_t* ctx, aoi_entity_t* entity, int x, int z) {
	entity->ocenter = entity->center;

	entity->center.x = x;
	entity->center.z = z;

	entity->node[0].shuffle(ctx, &entity->node[0], x);
	entity->node[1].shuffle(ctx, &entity->node[1], z);

	aoi_object_t* owner = entity->node[0].owner;

	for (aoi_object_t* node = ctx->enter_list.next; node != &ctx->enter_list; ) {
		node->enter_func(node->uid, owner->uid, node->ud);
#ifdef LINKAOI_HAVE_RESTORE_WITNESS
		hash_set_put(entity->witness, node->uid);
#endif
#ifdef LINKAOI_HAVE_RESTORE_VISIBLE
		hash_set_put(node->trigger->visible, owner->uid);
#endif
		aoi_object_t* tmp = node;
		node = node->next;
		tmp->next = tmp->prev = NULL;
		tmp->inout = 0;
	}

	ctx->enter_list.prev = ctx->enter_list.next = &ctx->enter_list;

	for (aoi_object_t* node = ctx->leave_list.next; node != &ctx->leave_list; ) {
		node->leave_func(node->uid, owner->uid, node->ud);
#ifdef LINKAOI_HAVE_RESTORE_WITNESS
		hash_set_del(entity->witness, node->uid);
#endif
#ifdef LINKAOI_HAVE_RESTORE_VISIBLE
		hash_set_del(node->trigger->visible, owner->uid);
#endif
		aoi_object_t* tmp = node;
		node = node->next;
		tmp->next = tmp->prev = NULL;
		tmp->inout = 0;
	}

	ctx->leave_list.prev = ctx->leave_list.next = &ctx->leave_list;
}

static void shuffle_trigger(aoi_context_t* ctx, aoi_trigger_t* trigger, int x, int z) {
	trigger->ocenter = trigger->center;

	trigger->center.x = x;
	trigger->center.z = z;

	if (trigger->ocenter.x < x) {
		trigger->node[2].shuffle(ctx, &trigger->node[2], x + trigger->range);
		trigger->node[0].shuffle(ctx, &trigger->node[0], x - trigger->range);
	} else {
		trigger->node[0].shuffle(ctx, &trigger->node[0], x - trigger->range);
		trigger->node[2].shuffle(ctx, &trigger->node[2], x + trigger->range);
	}

	if (trigger->ocenter.z < z) {
		trigger->node[3].shuffle(ctx, &trigger->node[3], z + trigger->range);
		trigger->node[1].shuffle(ctx, &trigger->node[1], z - trigger->range);
	} else {
		trigger->node[1].shuffle(ctx, &trigger->node[1], z - trigger->range);
		trigger->node[3].shuffle(ctx, &trigger->node[3], z + trigger->range);
	}

	aoi_object_t* owner = trigger->node[0].owner;
	for (aoi_object_t* node = ctx->enter_list.next; node != &ctx->enter_list; ) {
		owner->enter_func(owner->uid, node->uid, owner->ud);
#ifdef LINKAOI_HAVE_RESTORE_WITNESS
		hash_set_put(node->entity->witness, owner->uid);
#endif // LINKAOI_HAVE_RESTORE_WITNESS

#ifdef LINKAOI_HAVE_RESTORE_VISIBLE
		hash_set_put(owner->trigger->visible, node->uid);
#endif
		aoi_object_t* tmp = node;
		node = node->next;
		tmp->next = tmp->prev = NULL;
		tmp->inout = 0;
	}

	ctx->enter_list.prev = ctx->enter_list.next = &ctx->enter_list;

	for (aoi_object_t* node = ctx->leave_list.next; node != &ctx->leave_list; ) {
		owner->leave_func(owner->uid, node->uid, owner->ud);
#ifdef LINKAOI_HAVE_RESTORE_WITNESS
		hash_set_del(node->entity->witness, owner->uid);
#endif // LINKAOI_HAVE_RESTORE_WITNESS
#ifdef LINKAOI_HAVE_RESTORE_VISIBLE
		hash_set_del(owner->trigger->visible, node->uid);
#endif
		aoi_object_t* tmp = node;
		node = node->next;
		tmp->next = tmp->prev = NULL;
		tmp->inout = 0;
	}

	ctx->leave_list.prev = ctx->leave_list.next = &ctx->leave_list;
}

int create_entity(aoi_context_t* ctx, aoi_object_t* object, int x, int z) {
	if (object->entity) {
		return -1;
	}
	object->entity = malloc(sizeof(aoi_entity_t));
	memset(object->entity, 0, sizeof(aoi_entity_t));

#ifdef LINKAOI_HAVE_RESTORE_WITNESS
	object->entity->witness = hash_set_new();
#endif
	object->entity->center.x = UNLIMITED;
	object->entity->center.z = UNLIMITED;

	object->entity->node[0].owner = object;
	object->entity->node[1].owner = object;

	object->entity->node[0].flag |= AOI_ENTITY;
	object->entity->node[1].flag |= AOI_ENTITY;

	object->entity->node[0].shuffle = entity_shuffle_x;
	object->entity->node[1].shuffle = entity_shuffle_z;

	object->entity->node[0].order = 0;
	object->entity->node[1].order = 0;

	object->entity->node[0].pos.x = UNLIMITED;
	object->entity->node[0].pos.z = UNLIMITED;

	object->entity->node[1].pos.x = UNLIMITED;
	object->entity->node[1].pos.z = UNLIMITED;

	insert_node(ctx, 1, &object->entity->node[0]);
	insert_node(ctx, 0, &object->entity->node[1]);

	shuffle_entity(ctx, object->entity, x, z);
	return 0;
}

int create_trigger(aoi_context_t* ctx, aoi_object_t* object, int x, int z, int range, callback_func enter_func, callback_func leave_func, void* ud) {
	if (object->trigger) {
		return -1;
	}
	object->trigger = malloc(sizeof(aoi_trigger_t));
	memset(object->trigger, 0, sizeof(aoi_trigger_t));

#ifdef LINKAOI_HAVE_RESTORE_VISIBLE
	object->trigger->visible = hash_set_new();
#endif

	object->enter_func = enter_func;
	object->leave_func = leave_func;
	object->ud = ud;

	object->trigger->range = range;

	object->trigger->center.x = UNLIMITED;
	object->trigger->center.z = UNLIMITED;

	object->trigger->node[0].owner = object;
	object->trigger->node[1].owner = object;
	object->trigger->node[2].owner = object;
	object->trigger->node[3].owner = object;

	object->trigger->node[0].shuffle = trigger_low_bound_shuffle_x;
	object->trigger->node[1].shuffle = trigger_low_bound_shuffle_z;
	object->trigger->node[2].shuffle = trigger_high_bound_shuffle_x;
	object->trigger->node[3].shuffle = trigger_high_bound_shuffle_z;

	object->trigger->node[0].flag |= AOI_LOW_BOUND;
	object->trigger->node[1].flag |= AOI_LOW_BOUND;

	object->trigger->node[0].order = -2;
	object->trigger->node[1].order = -2;

	object->trigger->node[0].pos.x = UNLIMITED;
	object->trigger->node[0].pos.z = UNLIMITED;

	object->trigger->node[1].pos.x = UNLIMITED;
	object->trigger->node[1].pos.z = UNLIMITED;

	object->trigger->node[2].flag |= AOI_HIGH_BOUND;
	object->trigger->node[3].flag |= AOI_HIGH_BOUND;

	object->trigger->node[2].order = 2;
	object->trigger->node[3].order = 2;

	object->trigger->node[2].pos.x = UNLIMITED;
	object->trigger->node[2].pos.z = UNLIMITED;

	object->trigger->node[3].pos.x = UNLIMITED;
	object->trigger->node[3].pos.z = UNLIMITED;

	insert_node(ctx, 1, &object->trigger->node[0]);
	insert_node(ctx, 1, &object->trigger->node[2]);

	insert_node(ctx, 0, &object->trigger->node[1]);
	insert_node(ctx, 0, &object->trigger->node[3]);

	shuffle_trigger(ctx, object->trigger, x, z);

	return 0;
}

int delete_entity(aoi_context_t* ctx, aoi_object_t* object, int shuffle) {
	if (!object->entity) {
		return -1;
	}

	if (shuffle) {
		shuffle_entity(ctx, object->entity, UNLIMITED, UNLIMITED);
	}

	remove_node(&object->entity->node[0]);
	remove_node(&object->entity->node[1]);

#ifdef LINKAOI_HAVE_RESTORE_WITNESS
	hash_set_free(object->entity->witness);
#endif
	free(object->entity);
	object->entity = NULL;

	return 0;
}

int delete_trigger(aoi_context_t* ctx, aoi_object_t* object) {
	if (!object->trigger) {
		return -1;
	}
	remove_node(&object->trigger->node[0]);
	remove_node(&object->trigger->node[2]);

	remove_node(&object->trigger->node[1]);
	remove_node(&object->trigger->node[3]);

	free(object->trigger);
	object->trigger = NULL;

	return 0;
}

void move_entity(aoi_context_t* ctx, aoi_object_t* object, int x, int z) {
	shuffle_entity(ctx, object->entity, x, z);
}

void move_trigger(aoi_context_t* ctx, aoi_object_t* object, int x, int z) {
	shuffle_trigger(ctx, object->trigger, x, z);
}

aoi_context_t* create_aoi_ctx() {
	aoi_context_t* ctx = malloc(sizeof(*ctx));
	memset(ctx, 0, sizeof(*ctx));
	ctx->linklist[0].prev = ctx->linklist[0].next = &ctx->linklist[0];
	ctx->linklist[1].prev = ctx->linklist[1].next = &ctx->linklist[1];
	ctx->enter_list.prev = ctx->enter_list.next = &ctx->enter_list;
	ctx->leave_list.prev = ctx->leave_list.next = &ctx->leave_list;
	return ctx;
}

void release_aoi_ctx(aoi_context_t* ctx) {
	free(ctx);
}

aoi_object_t* create_aoi_object(aoi_context_t* ctx, int uid) {
	aoi_object_t* object = NULL;
	if (ctx->freelist) {
		object = ctx->freelist;
		ctx->freelist = object->next;
	} else {
		object = malloc(sizeof(*object));
	}
	memset(object, 0, sizeof(*object));
	object->uid = uid;

	return object;
}

void release_aoi_object(aoi_context_t* ctx, aoi_object_t* object) {
	delete_trigger(ctx, object);
	delete_entity(ctx, object, 0);
	object->next = ctx->freelist;
	ctx->freelist = object;
}

void foreach_aoi_entity(aoi_context_t* ctx, foreach_entity_func func, void* ud) {
	linknode_t* link = &ctx->linklist[0];
	for (linknode_t* node = link->next; node != link; node = node->next) {
		if (node->flag & AOI_ENTITY) {
			aoi_object_t* object = node->owner;
			func(object->uid, object->entity->center.x, object->entity->center.z, ud);
		}
	}
}

void foreach_aoi_trigger(aoi_context_t* ctx, foreach_trigger_func func, void* ud) {
	linknode_t* link = &ctx->linklist[0];
	for (linknode_t* node = link->next; node != link; node = node->next) {
		if (node->flag & AOI_LOW_BOUND) {
			aoi_object_t* object = node->owner;
			func(object->uid, object->trigger->center.x, object->trigger->center.z, object->trigger->range, ud);
		}
	}
}