#ifndef POOL_H
#define POOL_H

struct pool_ctx;

struct pool_ctx* pool_create(int size);
void pool_release(struct pool_ctx* ctx);
void* pool_malloc(struct pool_ctx* ctx);
void pool_free(struct pool_ctx* ctx, void* ptr);

#endif