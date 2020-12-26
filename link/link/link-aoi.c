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

#define LINKAOI_HAVE_RESTORE_WITNESS
#define LINKAOI_HAVE_RESTORE_VISIBLE

#ifdef _WIN32
#define inline __inline
#endif

struct aoi_entity;
struct aoi_trigger;
struct aoi_context;
struct linknode;

typedef void(*shuffle_func)(struct aoi_context*, struct linknode*, int);

typedef struct position {
	int x;
	int z;
} position_t;

typedef struct aoi_object {
	int uid;
	struct aoi_entity* entity;
	struct aoi_trigger* trigger;

	struct aoi_object* next;
	struct aoi_object* prev;

	callback_func entity_enter_func;
	callback_func entity_leave_func;

	callback_func trigger_enter_func;
	callback_func trigger_leave_func;

	int inout;
} aoi_object_t;

typedef struct linknode {
	aoi_object_t* owner;
	struct linknode* prev;
	struct linknode* next;
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

static inline void insert_node(aoi_context_t* aoi_ctx, int axis_x, linknode_t* node) {
	linknode_t* link;
	if (axis_x) {
		link = &aoi_ctx->linklist[0];
	} else {
		link = &aoi_ctx->linklist[1];
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


typedef int(*cmp_func)(position_t*, position_t*);

static inline void link_enter_list(aoi_context_t* aoi_ctx, aoi_object_t* self, aoi_object_t* other, cmp_func cmp, int is_entity) {
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

	aoi_object_t* prev = aoi_ctx->enter_list.prev;
	prev->next = other;
	other->next = &aoi_ctx->enter_list;
	other->prev = prev;
	aoi_ctx->enter_list.prev = other;

	other->inout = IN;
}

static inline void link_leave_list(aoi_context_t* aoi_ctx, aoi_object_t* self, aoi_object_t* other, cmp_func cmp, int is_entity) {
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

	aoi_object_t* prev = aoi_ctx->leave_list.prev;
	prev->next = other;
	other->next = &aoi_ctx->leave_list;
	other->prev = prev;
	aoi_ctx->leave_list.prev = other;

	other->inout = OUT;
}

static void entity_shuffle_x(aoi_context_t* aoi_ctx, linknode_t* node, int x) {
	node->pos.x = x;
	linknode_t* link = &aoi_ctx->linklist[0];
	if (link->next == link) {
		return;
	}

	while (node->prev != link && ((node->pos.x < node->prev->pos.x) || (node->pos.x == node->prev->pos.x && node->order <= node->prev->order))) {
		linknode_t* other = node->prev;
		exchange_node(node->prev, node);

		if (other->flag & AOI_LOW_BOUND) {
			link_leave_list(aoi_ctx, node->owner, other->owner, dt_z_range, 1);
		} else if (other->flag & AOI_HIGH_BOUND) {
			link_enter_list(aoi_ctx, node->owner, other->owner, dt_z_range, 1);
		}
	}

	while (node->next != link && ((node->pos.x > node->next->pos.x) || (node->pos.x == node->next->pos.x && node->order >= node->next->order))) {
		linknode_t* other = node->next;
		exchange_node(node, node->next);

		if (other->flag & AOI_LOW_BOUND) {
			link_enter_list(aoi_ctx, node->owner, other->owner, dt_z_range, 1);
		} else if (other->flag & AOI_HIGH_BOUND) {
			link_leave_list(aoi_ctx, node->owner, other->owner, dt_z_range, 1);
		}
	}
}

static void entity_shuffle_z(aoi_context_t* aoi_ctx, linknode_t* node, int z) {
	node->pos.z = z;
	linknode_t* link = &aoi_ctx->linklist[1];
	if (link->next == link) {
		return;
	}
	while (node->prev != link && ((node->pos.z < node->prev->pos.z) || (node->pos.z == node->prev->pos.z && node->order <= node->prev->order))) {
		linknode_t* other = node->prev;
		exchange_node(node->prev, node);

		if (other->flag & AOI_LOW_BOUND) {
			link_leave_list(aoi_ctx, node->owner, other->owner, dt_x_range, 1);
		} else if (other->flag & AOI_HIGH_BOUND) {
			link_enter_list(aoi_ctx, node->owner, other->owner, dt_x_range, 1);
		}
	}

	while (node->next != link && ((node->pos.z > node->next->pos.z) || (node->pos.z == node->next->pos.z && node->order >= node->next->order))) {
		linknode_t* other = node->next;
		exchange_node(node, node->next);

		if (other->flag & AOI_LOW_BOUND) {
			link_enter_list(aoi_ctx, node->owner, other->owner, dt_x_range, 1);
		} else if (other->flag & AOI_HIGH_BOUND) {
			link_leave_list(aoi_ctx, node->owner, other->owner, dt_x_range, 1);
		}
	}
}

static void trigger_low_bound_shuffle_x(aoi_context_t* aoi_ctx, linknode_t* node, int x) {
	node->pos.x = x;
	linknode_t* link = &aoi_ctx->linklist[0];
	if (link->next == link) {
		return;
	}

	while (node->prev != link && ((node->pos.x < node->prev->pos.x) || (node->pos.x == node->prev->pos.x && node->order <= node->prev->order))) {
		linknode_t* other = node->prev;
		exchange_node(node->prev, node);
		if (other->flag & AOI_ENTITY) {
			link_enter_list(aoi_ctx, node->owner, other->owner, dt_z_range, 0);
		}
	}

	while (node->next != link && ((node->pos.x > node->next->pos.x) || (node->pos.x == node->next->pos.x && node->order >= node->next->order))) {
		linknode_t* other = node->next;
		exchange_node(node, node->next);
		if (other->flag & AOI_ENTITY) {
			link_leave_list(aoi_ctx, node->owner, other->owner, dt_z_range, 0);
		}
	}
}

static void trigger_low_bound_shuffle_z(aoi_context_t* aoi_ctx, linknode_t* node, int z) {
	node->pos.z = z;
	linknode_t* link = &aoi_ctx->linklist[1];
	if (link->next == link) {
		return;
	}

	while (node->prev != link && ((node->pos.z < node->prev->pos.z) || (node->pos.z == node->prev->pos.z && node->order <= node->prev->order))) {
		linknode_t* other = node->prev;
		exchange_node(node->prev, node);
		if (other->flag & AOI_ENTITY) {
			link_enter_list(aoi_ctx, node->owner, other->owner, dt_x_range, 0);
		}
	}

	while (node->next != link && ((node->pos.z > node->next->pos.z) || (node->pos.z == node->next->pos.z && node->order >= node->next->order))) {
		linknode_t* other = node->next;
		exchange_node(node, node->next);
		if (other->flag & AOI_ENTITY) {
			link_leave_list(aoi_ctx, node->owner, other->owner, dt_x_range, 0);
		}
	}
}

static void trigger_high_bound_shuffle_x(aoi_context_t* aoi_ctx, linknode_t* node, int x) {
	node->pos.x = x;
	linknode_t* link = &aoi_ctx->linklist[0];
	if (link->next == link) {
		return;
	}

	while (node->prev != link && ((node->pos.x < node->prev->pos.x) || (node->pos.x == node->prev->pos.x && node->order <= node->prev->order))) {
		linknode_t* other = node->prev;
		exchange_node(node->prev, node);
		if (other->flag & AOI_ENTITY) {
			link_leave_list(aoi_ctx, node->owner, other->owner, dt_z_range, 0);
		}
	}

	while (node->next != link && ((node->pos.x > node->next->pos.x) || (node->pos.x == node->next->pos.x && node->order >= node->next->order))) {
		linknode_t* other = node->next;
		exchange_node(node, node->next);
		if (other->flag & AOI_ENTITY) {
			link_enter_list(aoi_ctx, node->owner, other->owner, dt_z_range, 0);
		}
	}
}

static void trigger_high_bound_shuffle_z(aoi_context_t* aoi_ctx, linknode_t* node, int z) {
	node->pos.z = z;
	linknode_t* link = &aoi_ctx->linklist[1];
	if (link->next == link) {
		return;
	}

	while (node->prev != link && ((node->pos.z < node->prev->pos.z) || (node->pos.z == node->prev->pos.z && node->order <= node->prev->order))) {
		linknode_t* other = node->prev;
		exchange_node(node->prev, node);
		if (other->flag & AOI_ENTITY) {
			link_leave_list(aoi_ctx, node->owner, other->owner, dt_x_range, 0);
		}
	}

	while (node->next != link && ((node->pos.z > node->next->pos.z) || (node->pos.z == node->next->pos.z && node->order >= node->next->order))) {
		linknode_t* other = node->next;
		exchange_node(node, node->next);
		if (other->flag & AOI_ENTITY) {
			link_enter_list(aoi_ctx, node->owner, other->owner, dt_x_range, 0);
		}
	}
}

static void shuffle_entity(aoi_context_t* aoi_ctx, aoi_entity_t* entity, int x, int z, void* ud) {
	entity->ocenter = entity->center;

	entity->center.x = x;
	entity->center.z = z;

	entity->node[0].shuffle(aoi_ctx, &entity->node[0], x);
	entity->node[1].shuffle(aoi_ctx, &entity->node[1], z);

	aoi_object_t* owner = entity->node[0].owner;

	for (aoi_object_t* node = aoi_ctx->enter_list.next; node != &aoi_ctx->enter_list; ) {
		owner->entity_enter_func(owner->uid, node->uid, ud);
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

	aoi_ctx->enter_list.prev = aoi_ctx->enter_list.next = &aoi_ctx->enter_list;

	for (aoi_object_t* node = aoi_ctx->leave_list.next; node != &aoi_ctx->leave_list; ) {
		owner->entity_leave_func(owner->uid, node->uid, ud);
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

	aoi_ctx->leave_list.prev = aoi_ctx->leave_list.next = &aoi_ctx->leave_list;
}

static void shuffle_trigger(aoi_context_t* aoi_ctx, aoi_trigger_t* trigger, int x, int z, void* ud) {
	trigger->ocenter = trigger->center;

	trigger->center.x = x;
	trigger->center.z = z;

	if (trigger->ocenter.x < x) {
		trigger->node[2].shuffle(aoi_ctx, &trigger->node[2], x + trigger->range);
		trigger->node[0].shuffle(aoi_ctx, &trigger->node[0], x - trigger->range);
	} else {
		trigger->node[0].shuffle(aoi_ctx, &trigger->node[0], x - trigger->range);
		trigger->node[2].shuffle(aoi_ctx, &trigger->node[2], x + trigger->range);
	}

	if (trigger->ocenter.z < z) {
		trigger->node[3].shuffle(aoi_ctx, &trigger->node[3], z + trigger->range);
		trigger->node[1].shuffle(aoi_ctx, &trigger->node[1], z - trigger->range);
	} else {
		trigger->node[1].shuffle(aoi_ctx, &trigger->node[1], z - trigger->range);
		trigger->node[3].shuffle(aoi_ctx, &trigger->node[3], z + trigger->range);
	}

	aoi_object_t* owner = trigger->node[0].owner;
	for (aoi_object_t* node = aoi_ctx->enter_list.next; node != &aoi_ctx->enter_list; ) {
		owner->trigger_enter_func(owner->uid, node->uid, ud);
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

	aoi_ctx->enter_list.prev = aoi_ctx->enter_list.next = &aoi_ctx->enter_list;

	for (aoi_object_t* node = aoi_ctx->leave_list.next; node != &aoi_ctx->leave_list; ) {
		owner->trigger_leave_func(owner->uid, node->uid, ud);
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

	aoi_ctx->leave_list.prev = aoi_ctx->leave_list.next = &aoi_ctx->leave_list;
}

int create_entity(aoi_context_t* aoi_ctx, aoi_object_t* aoi_object, int x, int z, callback_func enter_func, callback_func leave_func, void* ud) {
	if (aoi_object->entity) {
		return -1;
	}
	aoi_object->entity = malloc(sizeof(aoi_entity_t));
	memset(aoi_object->entity, 0, sizeof(aoi_entity_t));

	aoi_object->entity_enter_func = enter_func;
	aoi_object->entity_leave_func = leave_func;

#ifdef LINKAOI_HAVE_RESTORE_WITNESS
	aoi_object->entity->witness = hash_set_new();
#endif
	aoi_object->entity->center.x = UNLIMITED;
	aoi_object->entity->center.z = UNLIMITED;

	aoi_object->entity->node[0].owner = aoi_object;
	aoi_object->entity->node[1].owner = aoi_object;

	aoi_object->entity->node[0].flag |= AOI_ENTITY;
	aoi_object->entity->node[1].flag |= AOI_ENTITY;

	aoi_object->entity->node[0].shuffle = entity_shuffle_x;
	aoi_object->entity->node[1].shuffle = entity_shuffle_z;

	aoi_object->entity->node[0].order = 0;
	aoi_object->entity->node[1].order = 0;

	aoi_object->entity->node[0].pos.x = UNLIMITED;
	aoi_object->entity->node[0].pos.z = UNLIMITED;

	aoi_object->entity->node[1].pos.x = UNLIMITED;
	aoi_object->entity->node[1].pos.z = UNLIMITED;

	insert_node(aoi_ctx, 1, &aoi_object->entity->node[0]);
	insert_node(aoi_ctx, 0, &aoi_object->entity->node[1]);

	shuffle_entity(aoi_ctx, aoi_object->entity, x, z, ud);
	return 0;
}

int create_trigger(aoi_context_t* aoi_ctx, aoi_object_t* aoi_object, int x, int z, int range, callback_func enter_func, callback_func leave_func, void* ud) {
	if (aoi_object->trigger) {
		return -1;
	}
	aoi_object->trigger = malloc(sizeof(aoi_trigger_t));
	memset(aoi_object->trigger, 0, sizeof(aoi_trigger_t));

#ifdef LINKAOI_HAVE_RESTORE_VISIBLE
	aoi_object->trigger->visible = hash_set_new();
#endif

	aoi_object->trigger_enter_func = enter_func;
	aoi_object->trigger_leave_func = leave_func;

	aoi_object->trigger->range = range;

	aoi_object->trigger->center.x = UNLIMITED;
	aoi_object->trigger->center.z = UNLIMITED;

	aoi_object->trigger->node[0].owner = aoi_object;
	aoi_object->trigger->node[1].owner = aoi_object;
	aoi_object->trigger->node[2].owner = aoi_object;
	aoi_object->trigger->node[3].owner = aoi_object;

	aoi_object->trigger->node[0].shuffle = trigger_low_bound_shuffle_x;
	aoi_object->trigger->node[1].shuffle = trigger_low_bound_shuffle_z;
	aoi_object->trigger->node[2].shuffle = trigger_high_bound_shuffle_x;
	aoi_object->trigger->node[3].shuffle = trigger_high_bound_shuffle_z;

	aoi_object->trigger->node[0].flag |= AOI_LOW_BOUND;
	aoi_object->trigger->node[1].flag |= AOI_LOW_BOUND;

	aoi_object->trigger->node[0].order = -2;
	aoi_object->trigger->node[1].order = -2;

	aoi_object->trigger->node[0].pos.x = UNLIMITED;
	aoi_object->trigger->node[0].pos.z = UNLIMITED;

	aoi_object->trigger->node[1].pos.x = UNLIMITED;
	aoi_object->trigger->node[1].pos.z = UNLIMITED;

	aoi_object->trigger->node[2].flag |= AOI_HIGH_BOUND;
	aoi_object->trigger->node[3].flag |= AOI_HIGH_BOUND;

	aoi_object->trigger->node[2].order = 2;
	aoi_object->trigger->node[3].order = 2;

	aoi_object->trigger->node[2].pos.x = UNLIMITED;
	aoi_object->trigger->node[2].pos.z = UNLIMITED;

	aoi_object->trigger->node[3].pos.x = UNLIMITED;
	aoi_object->trigger->node[3].pos.z = UNLIMITED;

	insert_node(aoi_ctx, 1, &aoi_object->trigger->node[0]);
	insert_node(aoi_ctx, 1, &aoi_object->trigger->node[2]);

	insert_node(aoi_ctx, 0, &aoi_object->trigger->node[1]);
	insert_node(aoi_ctx, 0, &aoi_object->trigger->node[3]);

	shuffle_trigger(aoi_ctx, aoi_object->trigger, x, z, ud);

	return 0;
}

int delete_entity(aoi_context_t* aoi_ctx, aoi_object_t* aoi_object, int shuffle, void* ud) {
	if (!aoi_object->entity) {
		return -1;
	}

	if (shuffle) {
		shuffle_entity(aoi_ctx, aoi_object->entity, UNLIMITED, UNLIMITED, ud);
	}

	remove_node(&aoi_object->entity->node[0]);
	remove_node(&aoi_object->entity->node[1]);

#ifdef LINKAOI_HAVE_RESTORE_WITNESS
	hash_set_free(aoi_object->entity->witness);
#endif
	free(aoi_object->entity);
	aoi_object->entity = NULL;

	return 0;
}

int delete_trigger(aoi_context_t* aoi_ctx, aoi_object_t* aoi_object) {
	if (!aoi_object->trigger) {
		return -1;
	}
	remove_node(&aoi_object->trigger->node[0]);
	remove_node(&aoi_object->trigger->node[2]);

	remove_node(&aoi_object->trigger->node[1]);
	remove_node(&aoi_object->trigger->node[3]);

	free(aoi_object->trigger);
	aoi_object->trigger = NULL;

	return 0;
}

void move_entity(aoi_context_t* aoi_ctx, aoi_object_t* aoi_object, int x, int z, void* ud) {
	shuffle_entity(aoi_ctx, aoi_object->entity, x, z, ud);
}

void move_trigger(aoi_context_t* aoi_ctx, aoi_object_t* aoi_object, int x, int z, void* ud) {
	shuffle_trigger(aoi_ctx, aoi_object->trigger, x, z, ud);
}

struct aoi_context* create_aoi_ctx() {
	struct aoi_context* aoi_ctx = malloc(sizeof(*aoi_ctx));
	memset(aoi_ctx, 0, sizeof(*aoi_ctx));
	aoi_ctx->linklist[0].prev = aoi_ctx->linklist[0].next = &aoi_ctx->linklist[0];
	aoi_ctx->linklist[1].prev = aoi_ctx->linklist[1].next = &aoi_ctx->linklist[1];
	aoi_ctx->enter_list.prev = aoi_ctx->enter_list.next = &aoi_ctx->enter_list;
	aoi_ctx->leave_list.prev = aoi_ctx->leave_list.next = &aoi_ctx->leave_list;
	return aoi_ctx;
}

void release_aoi_ctx(aoi_context_t* aoi_ctx) {
	free(aoi_ctx);
}

struct aoi_object* create_aoi_object(aoi_context_t* aoi_ctx, int uid) {
	aoi_object_t* object = NULL;
	if (aoi_ctx->freelist) {
		object = aoi_ctx->freelist;
		aoi_ctx->freelist = object->next;
	} else {
		object = malloc(sizeof(*object));
	}
	memset(object, 0, sizeof(*object));
	object->uid = uid;

	return object;
}

void release_aoi_object(aoi_context_t* aoi_ctx, aoi_object_t* aoi_object) {
	delete_trigger(aoi_ctx, aoi_object);
	delete_entity(aoi_ctx, aoi_object, 0, NULL);
	aoi_object->next = aoi_ctx->freelist;
	aoi_ctx->freelist = aoi_object;
}

void foreach_aoi_entity(struct aoi_context* aoi_ctx, foreach_entity_func func, void* ud) {
	linknode_t* link = &aoi_ctx->linklist[0];
	for (linknode_t* node = link->next; node != link; node = node->next) {
		if (node->flag & AOI_ENTITY) {
			aoi_object_t* object = node->owner;
			func(object->uid, object->entity->center.x, object->entity->center.z, ud);
		}
	}
}

void foreach_aoi_trigger(struct aoi_context* aoi_ctx, foreach_trigger_func func, void* ud) {
	linknode_t* link = &aoi_ctx->linklist[0];
	for (linknode_t* node = link->next; node != link; node = node->next) {
		if (node->flag & AOI_LOW_BOUND) {
			aoi_object_t* object = node->owner;
			func(object->uid, object->trigger->center.x, object->trigger->center.z, object->trigger->range, ud);
		}
	}
}