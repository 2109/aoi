#ifndef SIMPLE_AOI_H
#define SIMPLE_AOI_H

#define LAYER_ITEM			0
#define LAYER_MONSTER		1
#define LAYER_USER			2
#define LAYER_MAX			3

#define ERROR_POS			-1
#define ERROR_LAYER			-2
#define ERROR_OBJECT_ID		-3

typedef void(*callback_func)(int self, int other, void* ud);
typedef void(*forearch_func)(int uid, float x, float z, void* ud);

struct aoi_context;

struct aoi_context* aoi_create(int, int, int, int, int, callback_func, callback_func);
void aoi_release(struct aoi_context*);

int aoi_enter(struct aoi_context*, int, float, float, int, void*);
int aoi_leave(struct aoi_context*, int, void*);
int aoi_update(struct aoi_context*, int, float, float, void*);
const char* aoi_error(int no);
void forearch_object(struct aoi_context*, forearch_func, void*);

#endif