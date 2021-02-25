#include "HashMap.h"

#include <assert.h>
#include <string.h>

#define FNV_OFFSET_BASIS	0x811c9dc5

size_t HashMap_Hash(void* data, size_t len) {
	size_t hash = FNV_OFFSET_BASIS;
	for (size_t i = 0; i < len; i++) {
		hash *= FNV_OFFSET_BASIS;
		hash ^= ((char*)data)[i];
	}
	return hash;
}

void* HashMap_KeyHashToValuePtr(HashMap* hashMap, size_t hash) {
	size_t start = hash % hashMap->capacity;
	size_t i = start;
	size_t* ihash;
	do {
		ihash = &((char*)hashMap->map)[i * hashMap->value_size];
		i = (i + 1) % hashMap->capacity;
	} while (*ihash != hash && *ihash != 0 && i != start);

	return *ihash == hash || *ihash == 0 ? ihash : NULL;
}

HashMap HashMap_New(size_t capacity, size_t key_size, size_t value_size) {
	HashMap hashMap;
	hashMap.capacity = capacity;
	hashMap.key_size = key_size;
	hashMap.value_size = sizeof(size_t) + value_size;
	hashMap.map = calloc(capacity, value_size);
	assert(hashMap.map != NULL);
	return hashMap;
}

void HashMap_Set(HashMap* hashMap, void* key, void* value) {
	size_t hash = HashMap_Hash(key, hashMap->key_size);
	size_t* valuePtr = HashMap_KeyHashToValuePtr(hashMap, hash);
	assert(valuePtr != NULL);
	*valuePtr = hash;
	memcpy(&((char*)valuePtr)[sizeof(size_t)], value, hashMap->value_size - sizeof(size_t));
}

void* HashMap_Get(HashMap* hashMap, void* key) {
	size_t hash = HashMap_Hash(key, hashMap->key_size);
	size_t* valuePtr = HashMap_KeyHashToValuePtr(hashMap, hash);
	assert(valuePtr != NULL);
	return *valuePtr == 0 ? NULL : valuePtr + 1;
}

int HashMap_Contains(HashMap* hashMap, void* key) {
	size_t hash = HashMap_Hash(key, hashMap->key_size);
	size_t* valuePtr = HashMap_KeyHashToValuePtr(hashMap, hash);
	assert(valuePtr != NULL);
	return *valuePtr != 0;
}

void HashMap_Remove(HashMap* hashMap, void* key) {
	size_t hash = HashMap_Hash(key, hashMap->key_size);
	size_t* valuePtr = HashMap_KeyHashToValuePtr(hashMap, hash);
	*valuePtr = 0;
}

void HashMap_Delete(HashMap* hashMap) {
	assert(hashMap->map != NULL);
	free(hashMap->map);
}