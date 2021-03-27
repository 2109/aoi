#include "hash_map.h"
#include "khash.h"
#include <assert.h>

KHASH_MAP_INIT_STR(hash_str, void*);
typedef khash_t(hash_str) hash_map_str_t;

hash_map_t hash_str_create() {
	return kh_init(hash_str);
}

void hash_str_release(hash_map_t hash) {
	kh_clear(hash_str, hash);
	kh_destroy(hash_str, hash);
}

void hash_str_set(hash_map_t hash, char* k, void* v) {
	int ok;
	khiter_t itr = kh_put(hash_str, hash, _strdup(k), &ok);
	assert(ok == 1 || ok == 2);
	kh_value((hash_map_str_t*)hash, itr) = v;
}

void hash_str_del(hash_map_t hash, char* k) {
	khiter_t itr = kh_get(hash_str, hash, k);
	assert(itr != kh_end((hash_map_str_t*)hash));
	kh_del(hash_str, hash, itr);
}

bool hash_str_exist(hash_map_t hash, char* k) {
	khiter_t itr = kh_get(hash_str, hash, k);
	return itr != kh_end((hash_map_str_t*)hash);
}

void* hash_str_find(hash_map_t hash, char* k) {
	khiter_t itr = kh_get(hash_str, hash, k);
	return itr == kh_end((hash_map_str_t*)hash) ? NULL : kh_value((hash_map_str_t*)hash, itr);
}

void hash_str_foreach(hash_map_t hash, hash_str_foreach_func func, void* ud) {
	const char* k = NULL;
	void* v = NULL;
	kh_foreach((hash_map_str_t*)hash, k, v, {
		func((char*)k, v, ud);
	});
}

size_t hash_str_size(hash_map_t hash) {
	return kh_size((hash_map_str_t*)hash);
}