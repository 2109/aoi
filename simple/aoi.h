#ifndef SIMPLE_AOI_H
#define SIMPLE_AOI_H


typedef void(*callback_func)( int self, int other, void* ud );
typedef void(*forearch_func)( int uid,float x,float z, void* ud );

struct aoi_context;

struct aoi_context* aoi_create(int, int, int, int, int, callback_func, callback_func);
void aoi_release(struct aoi_context*);

int aoi_enter(struct aoi_context*, int, float, float, int, void*);
int aoi_leave(struct aoi_context*, int, void*);
int aoi_update(struct aoi_context*, int, float, float, void*);
void forearch_object(struct aoi_context*, forearch_func, void*);

#endif