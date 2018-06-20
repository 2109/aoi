#ifndef TOWER_AOI_H
#define TOWER_AOI_H


typedef void(*callback_func)(int self, int other, int op, void* ud);

typedef void(*foreach_entity_func)(int uid, int x, int z, void* ud);
typedef void(*foreach_trigger_func)(int uid, int x, int z, int range, void* ud);

struct object;
struct aoi;

struct aoi* create_aoi(int max, int width, int height, int cell);
void release_aoi(struct aoi* aoi);

int create_entity(struct aoi* aoi, int uid, float x, float z, callback_func func, void* ud);
void remove_entity(struct aoi* aoi, int id);
void move_entity(struct aoi* aoi, int id, float nx, float nz);

int create_trigger(struct aoi* aoi, int uid, float x, float z, int range, callback_func func, void* ud);
void remove_trigger(struct aoi* aoi, int id);
void move_trigger(struct aoi* aoi, int id, float nx, float nz);

void foreach_entity(struct aoi* aoi, foreach_entity_func func, void* ud);
void foreach_trigger(struct aoi* aoi, foreach_trigger_func func, void* ud);

#endif