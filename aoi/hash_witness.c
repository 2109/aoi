#include "hash_witness.h"
#include <assert.h>

void hash_set_put(hash_set_t *self, int uid) {
	//assert(hash_set_has(self, uid) == 0);
	if ( hash_set_has(self, uid) == 1) {
		assert(0);
	}
	int ret;
	kh_put(uid, self, uid, &ret);
}

int hash_set_has(hash_set_t *self, int uid) {
	khiter_t k = kh_get(uid, self, uid);
	return k != kh_end(self);
}

void hash_set_del(hash_set_t *self, int uid) {
	//assert(hash_set_has(self, uid) == 1);
	if ( hash_set_has(self, uid) == 0 ) {
		assert(0);
	}
	khiter_t k = kh_get(uid, self, uid);
	kh_del(uid, self, k);
}