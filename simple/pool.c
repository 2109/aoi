#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
#define inline __inline
#endif
#define POOL_SIZE 64

struct pool_node_link {
	struct pool_node_link *next;
};


struct pool_obj {
	struct pool_obj* next;
};

struct pool_ctx {
	struct pool_node_link *pool;
	struct pool_obj *freelist;
	int size;
	int count;
};

static void
expand_pool(struct pool_ctx* ctx) {
	size_t alloc_size = sizeof(struct pool_node_link) + ctx->count * (sizeof(struct pool_obj) + ctx->size);
	struct pool_node_link* pool_link = malloc(alloc_size);
	memset(pool_link,0,alloc_size);

	pool_link->next = ctx->pool;
	ctx->pool = pool_link;

	struct pool_obj* pool = (struct pool_obj*)&pool_link[1];
	int i;
	for(i=0;i<ctx->count;i++) {
		struct pool_obj* obj_node = (struct pool_obj*)((char*)pool + i * (sizeof(struct pool_obj) + ctx->size));
		obj_node->next = ctx->freelist;
		ctx->freelist = obj_node;
	}
} 

struct pool_ctx*
pool_create(int size) {
	struct pool_ctx* ctx = malloc(sizeof(*ctx));
	memset(ctx,0,sizeof(*ctx));
	ctx->size = size;
	ctx->count = POOL_SIZE;

	expand_pool(ctx);

	return ctx;
}

void
pool_release(struct pool_ctx* ctx) {
	struct pool_node_link* cursor = ctx->pool;
	while(cursor) {
		struct pool_node_link* tmp = cursor;
		cursor = cursor->next;
		free(tmp);
	}
	free(ctx);
}

void*
pool_malloc(struct pool_ctx* ctx) {
	if (!ctx->freelist)
		expand_pool(ctx);

	struct pool_obj* node = ctx->freelist;
	ctx->freelist = node->next;
	return &node[1];
}

void
pool_free(struct pool_ctx* ctx,void* ptr) {
	struct pool_obj* node = (struct pool_obj*)((char*)ptr - sizeof(struct pool_obj));
	node->next = ctx->freelist;
	ctx->freelist = node;
}