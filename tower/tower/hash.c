#include "hash.h"
#include <assert.h>

void hash_set(hash_t* self, int uid, struct object* obj) {
	assert(hash_has(self, uid) == 0);
	int ok;
	khiter_t k = kh_put(watcher, self, uid, &ok);
	assert(ok == 1 || ok == 2);
	kh_value(self, k) = obj;
}

int hash_has(hash_t* self, int uid) {
	khiter_t k = kh_get(watcher, self, uid);
	if (k < kh_end(self)) {
		return kh_exist(self, k);
	}
	return 0;
}

void hash_del(hash_t* self, int uid) {
	assert(hash_has(self, uid) == 1);
	khiter_t k = kh_get(watcher, self, uid);
	assert(k != kh_end(self));
	kh_del(watcher, self, k);
}