#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdlib.h>

typedef struct {
	void* map;
	size_t capacity;
	size_t value_size;
	size_t key_size;
}HashMap;

HashMap HashMap_New(size_t capacity, size_t key_size, size_t value_size);

void HashMap_Set(HashMap* hashMap, void* key, void* value);

void* HashMap_Get(HashMap* hashMap, void* key);

int HashMap_Contains(HashMap* hashMap, void* key);

void HashMap_Remove(HashMap* hashMap, void* key);

void HashMap_Delete(HashMap* hashMap);

#endif