#include "hash.h"
#include <assert.h>

void hash_set(hash_t *self, int uid,struct object* obj) {
	int ok;
	khiter_t k = kh_put(watcher, self, uid, &ok);
	assert(ok == 1 || ok == 2);
	kh_value(self, k) = obj;
}

int hash_has(hash_t *self, int uid) {
	khiter_t k = kh_get(watcher, self, uid);
	return k != kh_end(self);
}

void hash_del(hash_t *self, int uid) {
	khiter_t k = kh_get(watcher, self, uid);
	assert(k != kh_end(self));
	kh_del(watcher, self, k);
}