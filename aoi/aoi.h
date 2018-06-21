#ifndef LINK_AOI_H
#define LINK_AOI_H


struct aoi_context;
struct aoi_object;

typedef void(*foreach_entity_func)( int uid, int x, int z, void* ud );
typedef void(*foreach_trigger_func)( int uid, int x, int z, int range, void* ud );

typedef void(*callback_func)( int self, int other, void* ud );

struct aoi_context* create_aoi_ctx();
struct aoi_object* create_aoi_object(struct aoi_context* aoi_ctx, int uid);

int create_entity(struct aoi_context* aoi_ctx, struct aoi_object* object, int x, int z, callback_func enter_func, callback_func leave_func, void *ud);
int create_trigger(struct aoi_context* aoi_ctx, struct aoi_object* aoi_object, int x, int z, int range, callback_func enter_func, callback_func leave_func, void *ud);

int delete_entity(struct aoi_context* aoi_ctx, struct aoi_object* aoi_object, void* ud);
int delete_trigger(struct aoi_context* aoi_ctx, struct aoi_object* aoi_object);

void move_entity(struct aoi_context* aoi_ctx, struct aoi_object* aoi_object, int x, int z, void* ud);
void move_trigger(struct aoi_context* aoi_ctx, struct aoi_object* aoi_object, int x, int z, void* ud);

void foreach_aoi_entity(struct aoi_context* aoi_ctx, foreach_entity_func func, void* ud);
void foreach_aoi_trigger(struct aoi_context* aoi_ctx, foreach_trigger_func func, void* ud);

#endif