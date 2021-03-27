#ifndef HASH_MAP_H
#define HASH_MAP_H
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
	typedef void* hash_map_t;
	typedef void(*hash_str_foreach_func)(char*, void*, void*);
	typedef void(*hash_int64_foreach_func)(int64_t, void*, void*);

	hash_map_t hash_str_create();
	void hash_str_release(hash_map_t hash);
	void hash_str_set(hash_map_t hash, char* k, void* v);
	void hash_str_del(hash_map_t hash, char* k);
	bool hash_str_exist(hash_map_t hash, char* k);
	void* hash_str_find(hash_map_t hash, char* k);
	void hash_str_foreach(hash_map_t hash, hash_str_foreach_func func, void* ud);
	size_t hash_str_size(hash_map_t hash);

	hash_map_t hash_int64_create();
	void hash_int64_release(hash_map_t hash);
	void hash_int64_set(hash_map_t hash, int64_t k, void* v);
	void hash_int64_del(hash_map_t hash, int64_t k);
	bool hash_int64_exist(hash_map_t hash, int64_t k);
	void* hash_int64_find(hash_map_t hash, int64_t k);
	void hash_int64_foreach(hash_map_t hash, hash_int64_foreach_func func, void* ud);
	size_t hash_int64_size(hash_map_t hash);
#ifdef __cplusplus
}
#endif
#endif