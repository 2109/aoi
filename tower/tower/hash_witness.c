#include "hash_witness.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void hash_set_put(hash_set_t *self, int uid, int self_uid, const char* debug) {
	//assert(hash_set_has(self, uid) == 0);
	if ( hash_set_has(self, uid) == 1) {
		//assert(0);
		printf("%s,duplex put:%d %d\n",debug, self_uid, uid);
	}
	int ret;
	kh_put(uid, self, uid, &ret);
}

int hash_set_has(hash_set_t *self, int uid) {
	khiter_t k = kh_get(uid, self, uid);
	if ( k < kh_end(self)) {
		return kh_exist(self, k);
	}
	return 0;
}

void hash_set_del(hash_set_t *self, int uid, int self_uid, const char* debug) {
	//assert(hash_set_has(self, uid) == 1);
	if ( hash_set_has(self, uid) == 0 ) {
		//assert(0);
		printf("%s,duplex del:%d %d\n", debug, self_uid, uid);
	}
	khiter_t k = kh_get(uid, self, uid);
	kh_del(uid, self, k);
}