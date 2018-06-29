#include "khash.h"



KHASH_SET_INIT_INT64(uid)

typedef khash_t(uid) hash_set_t;

#define hash_set_new() kh_init(uid)
#define hash_set_free(self) kh_destroy(uid, self)

#define hash_set_foreach(self, block) { \
	void *val; \
	for (khiter_t k = kh_begin(self); k < kh_end(self); ++k) {\
	if (!kh_exist(self, k)) continue; \
		val = kh_value(self, k); \
		block; \
	} \
}
void hash_set_put(hash_set_t *self, int);
int hash_set_has(hash_set_t *self, int);
void hash_set_del(hash_set_t *self, int);