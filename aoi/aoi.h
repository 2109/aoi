#ifndef LINK_AOI_H
#define LINK_AOI_H


struct aoi_context;
struct aoi_object;

typedef void(*foreach_entity_func)( int uid, int x, int z, void* ud );
typedef void(*foreach_trigger_func)( int uid, int x, int z, int range, void* ud );

typedef void(*entity_func)( int self, int other, int op, void* ud );
typedef void(*trigger_func)( int self, int other, int op, void* ud );

struct aoi_context* create_aoi_ctx(); 
struct aoi_object* create_aoi_object(struct aoi_context* aoi_ctx, int uid);
void create_entity(struct aoi_context* aoi_ctx, struct aoi_object* object, int x, int z, entity_func func, void *ud);
void create_trigger(struct aoi_context* aoi_ctx, struct aoi_object* aoi_object, int x, int z, int range, trigger_func func, void *ud);
void move_entity(struct aoi_context* aoi_ctx, struct aoi_object* aoi_object, int x, int z);
void move_trigger(struct aoi_context* aoi_ctx, struct aoi_object* aoi_object, int x, int z);
void foreach_aoi_entity(struct aoi_context* aoi_ctx, foreach_entity_func func, void* ud);
void foreach_aoi_trigger(struct aoi_context* aoi_ctx, foreach_trigger_func func, void* ud);

#endif