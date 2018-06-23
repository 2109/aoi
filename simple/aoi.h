#ifndef SIMPLE_AOI_H
#define SIMPLE_AOI_H
#include "object_container.h"

typedef void(*callback_func)( int self, int other, void* ud );

struct aoi_context;

struct aoi_context* aoi_create(int, int, int, int, int, callback_func, callback_func);
void aoi_release(struct aoi_context*);

int aoi_enter(struct aoi_context*, int, float, float, int, void*);
int aoi_leave(struct aoi_context*, int, void*);
int aoi_update(struct aoi_context*, int, float, float, void*);
void forearch_object(struct aoi_context*, foreach_func, void*);

#endif