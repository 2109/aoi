#include "hash_set.h"


void hash_set_put(hash_set_t *self, int uid) {
	int ret;
	kh_put(uid, self, uid, &ret);
}

int hash_set_has(hash_set_t *self, int uid) {
	khiter_t k = kh_get(uid, self, uid);
	return k != kh_end(self);
}

void hash_set_del(hash_set_t *self, int uid) {
	khiter_t k = kh_get(uid, self, uid);
	kh_del(uid, self, k);
}