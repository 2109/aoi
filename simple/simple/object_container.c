#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "object_container.h"

#ifdef _WIN32
#define inline __inline
#endif

struct object_sot {
	int index;
	struct object_sot* next;
};

struct object_container {
	void** mgr;
	int size;
	struct object_sot* slots;
	struct object_sot* freelist;
};


struct object_container*
	container_create(int max) {
	assert(max > 0);

	struct object_container* container = malloc(sizeof(*container));
	memset(container, 0, sizeof(*container));

	container->size = max;
	container->mgr = malloc(sizeof(*container->mgr) * container->size);
	memset(container->mgr, 0, sizeof(*container->mgr) * container->size);

	container->slots = malloc(sizeof(*container->slots) * container->size);
	memset(container->slots, 0, sizeof(*container->slots) * container->size);
	int i;
	for (i = 0; i < container->size; i++) {
		struct object_sot* slot = &container->slots[i];
		slot->index = i;
		slot->next = container->freelist;
		container->freelist = slot;
	}

	return container;
}

static void
container_resize(struct object_container* container) {
	int nsize = container->size * 2;

	void** omgr = container->mgr;
	container->mgr = malloc(sizeof(*container->mgr) * nsize);
	memset(container->mgr, 0, sizeof(*container->mgr) * nsize);
	memcpy(container->mgr, omgr, container->size * sizeof(*container->mgr));
	free(omgr);

	struct object_sot* oslots = container->slots;
	container->slots = malloc(sizeof(*container->slots) * nsize);
	memset(container->slots, 0, sizeof(*container->slots) * nsize);
	memcpy(container->slots, oslots, container->size * sizeof(*container->slots));
	free(oslots);

	int i;
	for (i = container->size; i < nsize; i++) {
		struct object_sot* slot = &container->slots[i];
		slot->index = i;
		slot->next = container->freelist;
		container->freelist = slot;
	}

	container->size = nsize;
}

void
container_release(struct object_container* container) {
	free(container->mgr);
	free(container->slots);
	free(container);
}

void*
container_get(struct object_container* container, int id) {
	if (id < 0 || id >= container->size) {
		return NULL;
	}
	void* object = container->mgr[id];
	return object;
}

void
container_foreach(struct object_container* container, foreach_func func, void* ud) {
	int i;
	for (i = 0; i < container->size; i++) {
		if (container->mgr[i]) {
			func(i, container->mgr[i], ud);
		}
	}
}

int
container_add(struct object_container* container, void* object) {
	if (!container->freelist) {
		container_resize(container);
	}

	struct object_sot* slot = container->freelist;
	container->freelist = slot->next;

	container->mgr[slot->index] = object;
	return slot->index;
}

void
container_remove(struct object_container* container, int id) {
	struct object_sot* slot = &container->slots[id];
	slot->next = container->freelist;
	container->freelist = slot;
	container->mgr[id] = NULL;
}