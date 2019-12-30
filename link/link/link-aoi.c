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

#define RESTORE_WITNESS
#define RESTORE_VISIBLE

#ifdef _WIN32
#define inline __inline
#endif

struct aoi_entity;
struct aoi_trigger;

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
	uint8_t flag;
	int8_t order;
} linknode_t;

typedef struct linklist {
	linknode_t* head;
	linknode_t* tail;
} linklist_t;

typedef struct aoi_entity {
	position_t center;
	position_t ocenter;
	linknode_t node[2];
#ifdef RESTORE_WITNESS
	hash_set_t* witness;
#endif
} aoi_entity_t;

typedef struct aoi_trigger {
	position_t center;
	position_t ocenter;
	linknode_t node[4];
	int range;
#ifdef RESTORE_VISIBLE
	hash_set_t* visible;
#endif
} aoi_trigger_t;

typedef struct aoi_context {
	linklist_t linklist[2];

	aoi_object_t* freelist;

	struct aoi_object* enter;
	struct aoi_object* leave;
} aoi_context_t;

void
insert_node(aoi_context_t* aoi_ctx, int axis_x, linknode_t* linknode) {
	linklist_t* first;
	if (axis_x) {
		first = &aoi_ctx->linklist[0];
	} else {
		first = &aoi_ctx->linklist[1];
	}
	if (first->head == NULL) {
		assert(first->head == first->tail);
		first->head = first->tail = linknode;
	} else {
		if (first->head == first->tail) {
			first->head = linknode;
			linknode->next = first->tail;
			first->tail->prev = linknode;
		} else {
			linknode->next = first->head;
			first->head->prev = linknode;
			first->head = linknode;
		}
	}
}

void
remove_node(aoi_context_t* aoi_ctx, int axis_x, linknode_t* linknode) {
	linklist_t* first;
	if (axis_x) {
		first = &aoi_ctx->linklist[0];
	} else {
		first = &aoi_ctx->linklist[1];
	}

	if (first->head == first->tail) {
		assert(linknode == first->head);
		first->head = first->tail = NULL;
	} else {
		if (linknode->prev) {
			linknode->prev->next = linknode->next;
		}
		if (linknode->next) {
			linknode->next->prev = linknode->prev;
		}

		if (linknode == first->head) {
			first->head = linknode->next;
		} else if (linknode == first->tail) {
			first->tail = linknode->prev;
		}
	}
	linknode->next = linknode->prev = NULL;
}

static inline int
dt_x_range(position_t* entity, position_t* trigger) {
	return abs(entity->x - trigger->x);
}

static inline int
dt_z_range(position_t* entity, position_t* trigger) {
	return abs(entity->z - trigger->z);
}

typedef int(*cmp_func)(position_t*, position_t*);

static inline void
link_enter_result(aoi_context_t* aoi_ctx, aoi_object_t* self, aoi_object_t* other, cmp_func cmp, int is_entity) {
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

	if (!aoi_ctx->enter) {
		aoi_ctx->enter = other;
	} else {
		aoi_ctx->enter->prev = other;
		other->next = aoi_ctx->enter;
		aoi_ctx->enter = other;
	}
	other->inout = IN;
}

static inline void
link_leave_result(aoi_context_t* aoi_ctx, aoi_object_t* self, aoi_object_t* other, cmp_func cmp, int is_entity) {
	if (other->inout == OUT) {
		return;
	}

	if (other->inout == IN) {
		if (aoi_ctx->enter == other) {
			aoi_ctx->enter = other->next;
		}

		if (other->next) {
			other->next->prev = other->prev;
		}

		if (other->prev) {
			other->prev->next = other->next;
		}

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

	if (!aoi_ctx->leave) {
		aoi_ctx->leave = other;
	} else {
		aoi_ctx->leave->prev = other;
		other->next = aoi_ctx->leave;
		aoi_ctx->leave = other;
	}
	other->inout = OUT;
}

static inline void
exchange(linklist_t* first, linknode_t* A, linknode_t* B) {
	A->next = B->next;
	if (B->next) {
		B->next->prev = A;
	}

	linknode_t* tmp_prev = A->prev;
	B->prev = tmp_prev;
	if (tmp_prev) {
		tmp_prev->next = B;
	}

	B->next = A;
	A->prev = B;

	if (first->head == A) {
		first->head = B;
	} else if (first->head == B) {
		first->head = A;
	}

	if (first->tail == A) {
		first->tail = B;
	} else if (first->tail == B) {
		first->tail = A;
	}
}

void
shuffle_x(aoi_context_t* aoi_ctx, linknode_t* node, int x) {
	linklist_t* first = &aoi_ctx->linklist[0];
	node->pos.x = x;
	if (first->head == first->tail)
		return;

	if (node->flag & AOI_ENTITY) {
		while (node->prev != NULL && ((node->pos.x < node->prev->pos.x) || (node->pos.x == node->prev->pos.x && node->order <= node->prev->order))) {
			linknode_t* other = node->prev;
			exchange(first, node->prev, node);

			if (other->flag & AOI_LOW_BOUND) {
				link_leave_result(aoi_ctx, node->owner, other->owner, dt_z_range, 1);
			} else if (other->flag & AOI_HIGH_BOUND) {
				link_enter_result(aoi_ctx, node->owner, other->owner, dt_z_range, 1);
			}
		}

		while (node->next != NULL && ((node->pos.x > node->next->pos.x) || (node->pos.x == node->next->pos.x && node->order >= node->next->order))) {
			linknode_t* other = node->next;
			exchange(first, node, node->next);

			if (other->flag & AOI_LOW_BOUND) {
				link_enter_result(aoi_ctx, node->owner, other->owner, dt_z_range, 1);
			} else if (other->flag & AOI_HIGH_BOUND) {
				link_leave_result(aoi_ctx, node->owner, other->owner, dt_z_range, 1);
			}
		}
	} else if (node->flag & AOI_LOW_BOUND) {
		while (node->prev != NULL && ((node->pos.x < node->prev->pos.x) || (node->pos.x == node->prev->pos.x && node->order <= node->prev->order))) {
			linknode_t* other = node->prev;
			exchange(first, node->prev, node);
			if (other->flag & AOI_ENTITY) {
				link_enter_result(aoi_ctx, node->owner, other->owner, dt_z_range, 0);
			}
		}

		while (node->next != NULL && ((node->pos.x > node->next->pos.x) || (node->pos.x == node->next->pos.x && node->order >= node->next->order))) {
			linknode_t* other = node->next;
			exchange(first, node, node->next);
			if (other->flag & AOI_ENTITY) {
				link_leave_result(aoi_ctx, node->owner, other->owner, dt_z_range, 0);
			}
		}
	} else if (node->flag & AOI_HIGH_BOUND) {
		while (node->prev != NULL && ((node->pos.x < node->prev->pos.x) || (node->pos.x == node->prev->pos.x && node->order <= node->prev->order))) {
			linknode_t* other = node->prev;
			exchange(first, node->prev, node);
			if (other->flag & AOI_ENTITY) {
				link_leave_result(aoi_ctx, node->owner, other->owner, dt_z_range, 0);
			}
		}

		while (node->next != NULL && ((node->pos.x > node->next->pos.x) || (node->pos.x == node->next->pos.x && node->order >= node->next->order))) {
			linknode_t* other = node->next;
			exchange(first, node, node->next);
			if (other->flag & AOI_ENTITY) {
				link_enter_result(aoi_ctx, node->owner, other->owner, dt_z_range, 0);
			}
		}
	}
}

void
shuffle_z(aoi_context_t* aoi_ctx, linknode_t* node, int z) {
	linklist_t* first = &aoi_ctx->linklist[1];
	node->pos.z = z;
	if (first->head == first->tail)
		return;

	if (node->flag & AOI_ENTITY) {
		while (node->prev != NULL && ((node->pos.z < node->prev->pos.z) || (node->pos.z == node->prev->pos.z && node->order <= node->prev->order))) {
			linknode_t* other = node->prev;
			exchange(first, node->prev, node);

			if (other->flag & AOI_LOW_BOUND) {
				link_leave_result(aoi_ctx, node->owner, other->owner, dt_x_range, 1);
			} else if (other->flag & AOI_HIGH_BOUND) {
				link_enter_result(aoi_ctx, node->owner, other->owner, dt_x_range, 1);
			}
		}

		while (node->next != NULL && ((node->pos.z > node->next->pos.z) || (node->pos.z == node->next->pos.z && node->order >= node->next->order))) {
			linknode_t* other = node->next;
			exchange(first, node, node->next);

			if (other->flag & AOI_LOW_BOUND) {
				link_enter_result(aoi_ctx, node->owner, other->owner, dt_x_range, 1);
			} else if (other->flag & AOI_HIGH_BOUND) {
				link_leave_result(aoi_ctx, node->owner, other->owner, dt_x_range, 1);
			}
		}
	} else if (node->flag & AOI_LOW_BOUND) {
		while (node->prev != NULL && ((node->pos.z < node->prev->pos.z) || (node->pos.z == node->prev->pos.z && node->order <= node->prev->order))) {
			linknode_t* other = node->prev;
			exchange(first, node->prev, node);
			if (other->flag & AOI_ENTITY) {
				link_enter_result(aoi_ctx, node->owner, other->owner, dt_x_range, 0);
			}
		}

		while (node->next != NULL && ((node->pos.z > node->next->pos.z) || (node->pos.z == node->next->pos.z && node->order >= node->next->order))) {
			linknode_t* other = node->next;
			exchange(first, node, node->next);
			if (other->flag & AOI_ENTITY) {
				link_leave_result(aoi_ctx, node->owner, other->owner, dt_x_range, 0);
			}
		}
	} else if (node->flag & AOI_HIGH_BOUND) {
		while (node->prev != NULL && ((node->pos.z < node->prev->pos.z) || (node->pos.z == node->prev->pos.z && node->order <= node->prev->order))) {
			linknode_t* other = node->prev;
			exchange(first, node->prev, node);
			if (other->flag & AOI_ENTITY) {
				link_leave_result(aoi_ctx, node->owner, other->owner, dt_x_range, 0);
			}
		}

		while (node->next != NULL && ((node->pos.z > node->next->pos.z) || (node->pos.z == node->next->pos.z && node->order >= node->next->order))) {
			linknode_t* other = node->next;
			exchange(first, node, node->next);
			if (other->flag & AOI_ENTITY) {
				link_enter_result(aoi_ctx, node->owner, other->owner, dt_x_range, 0);
			}
		}
	}
}

void
shuffle_entity(aoi_context_t* aoi_ctx, aoi_entity_t* entity, int x, int z, void* ud) {
	entity->ocenter = entity->center;

	entity->center.x = x;
	entity->center.z = z;

	shuffle_x(aoi_ctx, &entity->node[0], x);
	shuffle_z(aoi_ctx, &entity->node[1], z);

	aoi_object_t* owner = entity->node[0].owner;
	aoi_object_t* cursor = aoi_ctx->enter;
	while (cursor) {
		owner->entity_enter_func(owner->uid, cursor->uid, ud);
#ifdef RESTORE_WITNESS
		hash_set_put(entity->witness, cursor->uid);
#endif
#ifdef RESTORE_VISIBLE
		hash_set_put(cursor->trigger->visible, owner->uid);
#endif
		aoi_object_t* tmp = cursor;
		cursor = cursor->next;
		tmp->next = tmp->prev = NULL;
		tmp->inout = 0;
	}

	aoi_ctx->enter = NULL;

	cursor = aoi_ctx->leave;
	while (cursor) {
		owner->entity_leave_func(owner->uid, cursor->uid, ud);
#ifdef RESTORE_WITNESS
		hash_set_del(entity->witness, cursor->uid);
#endif
#ifdef RESTORE_VISIBLE
		hash_set_del(cursor->trigger->visible, owner->uid);
#endif
		aoi_object_t* tmp = cursor;
		cursor = cursor->next;
		tmp->next = tmp->prev = NULL;
		tmp->inout = 0;
	}

	aoi_ctx->leave = NULL;
}

void
shuffle_trigger(aoi_context_t* aoi_ctx, aoi_trigger_t* trigger, int x, int z, void* ud) {
	trigger->ocenter = trigger->center;

	trigger->center.x = x;
	trigger->center.z = z;

	if (trigger->ocenter.x < x) {
		shuffle_x(aoi_ctx, &trigger->node[2], x + trigger->range);
		shuffle_x(aoi_ctx, &trigger->node[0], x - trigger->range);
	} else {
		shuffle_x(aoi_ctx, &trigger->node[0], x - trigger->range);
		shuffle_x(aoi_ctx, &trigger->node[2], x + trigger->range);
	}

	if (trigger->ocenter.z < z) {
		shuffle_z(aoi_ctx, &trigger->node[3], z + trigger->range);
		shuffle_z(aoi_ctx, &trigger->node[1], z - trigger->range);
	} else {
		shuffle_z(aoi_ctx, &trigger->node[1], z - trigger->range);
		shuffle_z(aoi_ctx, &trigger->node[3], z + trigger->range);
	}

	aoi_object_t* owner = trigger->node[0].owner;
	aoi_object_t* cursor = aoi_ctx->enter;
	while (cursor) {
		owner->trigger_enter_func(owner->uid, cursor->uid, ud);
#ifdef RESTORE_WITNESS
		hash_set_put(cursor->entity->witness, owner->uid);
#endif // RESTORE_WITNESS

#ifdef RESTORE_VISIBLE
		hash_set_put(owner->trigger->visible, cursor->uid);
#endif

		aoi_object_t* tmp = cursor;
		cursor = cursor->next;
		tmp->next = tmp->prev = NULL;
		tmp->inout = 0;
	}

	aoi_ctx->enter = NULL;

	cursor = aoi_ctx->leave;
	while (cursor) {
		owner->trigger_leave_func(owner->uid, cursor->uid, ud);
#ifdef RESTORE_WITNESS
		hash_set_del(cursor->entity->witness, owner->uid);
#endif // RESTORE_WITNESS
#ifdef RESTORE_VISIBLE
		hash_set_del(owner->trigger->visible, cursor->uid);
#endif
		aoi_object_t* tmp = cursor;
		cursor = cursor->next;
		tmp->next = tmp->prev = NULL;
		tmp->inout = 0;
	}

	aoi_ctx->leave = NULL;
}

int
create_entity(aoi_context_t* aoi_ctx, aoi_object_t* aoi_object, int x, int z, callback_func enter_func, callback_func leave_func, void* ud) {
	if (aoi_object->entity) {
		return -1;
	}
	aoi_object->entity = malloc(sizeof(aoi_entity_t));
	memset(aoi_object->entity, 0, sizeof(aoi_entity_t));

	aoi_object->entity_enter_func = enter_func;
	aoi_object->entity_leave_func = leave_func;

#ifdef RESTORE_WITNESS
	aoi_object->entity->witness = hash_set_new();
#endif
	aoi_object->entity->center.x = UNLIMITED;
	aoi_object->entity->center.z = UNLIMITED;

	aoi_object->entity->node[0].owner = aoi_object;
	aoi_object->entity->node[1].owner = aoi_object;

	aoi_object->entity->node[0].flag |= AOI_ENTITY;
	aoi_object->entity->node[1].flag |= AOI_ENTITY;

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

int
create_trigger(aoi_context_t* aoi_ctx, aoi_object_t* aoi_object, int x, int z, int range, callback_func enter_func, callback_func leave_func, void* ud) {
	if (aoi_object->trigger) {
		return -1;
	}
	aoi_object->trigger = malloc(sizeof(aoi_trigger_t));
	memset(aoi_object->trigger, 0, sizeof(aoi_trigger_t));

#ifdef RESTORE_VISIBLE
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

int
delete_entity(aoi_context_t* aoi_ctx, aoi_object_t* aoi_object, int shuffle, void* ud) {
	if (!aoi_object->entity) {
		return -1;
	}

	if (shuffle) {
		shuffle_entity(aoi_ctx, aoi_object->entity, UNLIMITED, UNLIMITED, ud);
	}

	remove_node(aoi_ctx, 1, &aoi_object->entity->node[0]);
	remove_node(aoi_ctx, 0, &aoi_object->entity->node[1]);

#ifdef RESTORE_WITNESS
	hash_set_free(aoi_object->entity->witness);
#endif
	free(aoi_object->entity);
	aoi_object->entity = NULL;

	return 0;
}

int
delete_trigger(aoi_context_t* aoi_ctx, aoi_object_t* aoi_object) {
	if (!aoi_object->trigger) {
		return -1;
	}
	remove_node(aoi_ctx, 1, &aoi_object->trigger->node[0]);
	remove_node(aoi_ctx, 1, &aoi_object->trigger->node[2]);

	remove_node(aoi_ctx, 0, &aoi_object->trigger->node[1]);
	remove_node(aoi_ctx, 0, &aoi_object->trigger->node[3]);

	free(aoi_object->trigger);
	aoi_object->trigger = NULL;

	return 0;
}

void
move_entity(aoi_context_t* aoi_ctx, aoi_object_t* aoi_object, int x, int z, void* ud) {
	shuffle_entity(aoi_ctx, aoi_object->entity, x, z, ud);
}

void
move_trigger(aoi_context_t* aoi_ctx, aoi_object_t* aoi_object, int x, int z, void* ud) {
	shuffle_trigger(aoi_ctx, aoi_object->trigger, x, z, ud);
}

struct aoi_context*
	create_aoi_ctx() {
	struct aoi_context* aoi_ctx = malloc(sizeof(*aoi_ctx));
	memset(aoi_ctx, 0, sizeof(*aoi_ctx));
	return aoi_ctx;
}

void
release_aoi_ctx(aoi_context_t* aoi_ctx) {
	free(aoi_ctx);
}

struct aoi_object*
	create_aoi_object(aoi_context_t* aoi_ctx, int uid) {
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

void
release_aoi_object(aoi_context_t* aoi_ctx, aoi_object_t* aoi_object) {
	delete_trigger(aoi_ctx, aoi_object);
	delete_entity(aoi_ctx, aoi_object, 0, NULL);
	aoi_object->next = aoi_ctx->freelist;
	aoi_ctx->freelist = aoi_object;
}

void
foreach_aoi_entity(struct aoi_context* aoi_ctx, foreach_entity_func func, void* ud) {
	linklist_t* list = &aoi_ctx->linklist[0];
	linknode_t* cursor = list->head;
	while (cursor) {
		if (cursor->flag & AOI_ENTITY) {
			aoi_object_t* object = cursor->owner;
			func(object->uid, object->entity->center.x, object->entity->center.z, ud);
		}
		cursor = cursor->next;
	}
}

void
foreach_aoi_trigger(struct aoi_context* aoi_ctx, foreach_trigger_func func, void* ud) {
	linklist_t* list = &aoi_ctx->linklist[0];
	linknode_t* cursor = list->head;
	while (cursor) {
		if (cursor->flag & AOI_LOW_BOUND) {
			aoi_object_t* object = cursor->owner;
			func(object->uid, object->trigger->center.x, object->trigger->center.z, object->trigger->range, ud);
		}
		cursor = cursor->next;
	}
}