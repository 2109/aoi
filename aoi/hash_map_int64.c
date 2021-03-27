#include "hash_map.h"
#include "khash.h"
#include <assert.h>

KHASH_MAP_INIT_INT64(hash_int64, void*);
typedef khash_t(hash_int64) hash_map_int64_t;

hash_map_t hash_int64_create() {
	return kh_init(hash_int64);
}

void hash_int64_release(hash_map_t hash) {
	kh_clear(hash_int64, hash);
	kh_destroy(hash_int64, hash);
}

void hash_int64_set(hash_map_t hash, int64_t k, void* v) {
	int ok;
	khiter_t itr = kh_put(hash_int64, hash, k, &ok);
	assert(ok == 1 || ok == 2);
	kh_value((hash_map_int64_t*)hash, itr) = v;
}

void hash_int64_del(hash_map_t hash, int64_t k) {
	khiter_t itr = kh_get(hash_int64, hash, k);
	assert(itr != kh_end((hash_map_int64_t*)hash));
	kh_del(hash_int64, hash, itr);
}

bool hash_int64_exist(hash_map_t hash, int64_t k) {
	khiter_t itr = kh_get(hash_int64, hash, k);
	return itr != kh_end((hash_map_int64_t*)hash);
}

void* hash_int64_find(hash_map_t hash, int64_t k) {
	khiter_t itr = kh_get(hash_int64, hash, k);
	return itr == kh_end((hash_map_int64_t*)hash) ? NULL : kh_value((hash_map_int64_t*)hash, itr);
}

void hash_int64_foreach(hash_map_t hash, hash_int64_foreach_func func, void* ud) {
	int64_t k = NULL;
	void* v = NULL;
	kh_foreach((hash_map_int64_t*)hash, k, v, {
		func(k, v, ud);
	});
}

size_t hash_int64_size(hash_map_t hash) {
	return kh_size((hash_map_int64_t*)hash);
}