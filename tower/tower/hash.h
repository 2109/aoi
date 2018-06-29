#ifndef HASH_H
#define HASH_H

#include "khash.h"


struct object;

KHASH_MAP_INIT_INT(watcher, struct object*);

typedef khash_t(watcher) hash_t;

#define hash_new() kh_init(watcher)
#define hash_free(self) kh_destroy(watcher, self)
#define hash_foreach(self, k, v, code) kh_foreach(self, k, v, code)

void hash_set(hash_t *self, int, struct object*);
int hash_has(hash_t *self, int);
void hash_del(hash_t *self, int);

#endif