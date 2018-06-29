#ifndef CONTAINER_H
#define CONTAINER_H


struct object_container;

typedef void (*foreach_func)(int id,void* data,void* ud);

struct object_container* container_create(int max);
void container_release(struct object_container* container);
void* container_get(struct object_container* container,int id);
int container_add(struct object_container* container,void* object);
void container_remove(struct object_container* container,int id);
void container_foreach(struct object_container* container,foreach_func func,void* ud);

#endif