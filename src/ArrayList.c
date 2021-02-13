#include "ArrayList.h"

#include <string.h>
#include <assert.h>

ArrayList ArrayList_New(size_t initialCapacity, size_t elementSize) {
	ArrayList list;
	list.ptr = malloc(elementSize * initialCapacity);
	list.capacity = initialCapacity;
	list.size = 0;
	list.elsize = elementSize;
	return list;
}

ArrayList ArrayList_NewFromArray(void* array, size_t size, size_t elementSize) {
	ArrayList list;
	list.ptr = malloc(elementSize * size);
	list.capacity = size * 2;
	list.size = size;
	list.elsize = elementSize;
	memcpy(list.ptr, array, size * elementSize);
	return list;
}

void ArrayList_Set(ArrayList* arrayList, size_t index, void* element) {
	assert(index < arrayList->size);
	memcpy((char*)(arrayList->ptr) + index * arrayList->elsize, element, arrayList->elsize);
}

void* ArrayList_Get(ArrayList* arrayList, size_t index) {
	assert(index < arrayList->size);
	return (void*)((char*)(arrayList->ptr) + index * arrayList->elsize);
}

void ArrayList_Add(ArrayList* arrayList, void* element) {
	arrayList->size++;
	if (arrayList->size >= arrayList->capacity) {
		arrayList->capacity *= 2;
		arrayList->ptr = realloc(arrayList->ptr, arrayList->elsize * arrayList->capacity);
	}
	ArrayList_Set(arrayList, arrayList->size - 1, element);
}

void ArrayList_Remove(ArrayList* arrayList, size_t index) {
	assert(index < arrayList->size);
	void* dst = (void*)((char*)(arrayList->ptr) + index * arrayList->elsize);
	void* src = (void*)(((char*)dst) + arrayList->elsize);
	memmove(dst, src, (arrayList->size - 1 - index) * arrayList->elsize);
	arrayList->size--;
}

void ArrayList_Clear(ArrayList* arrayList) {
	arrayList->size = 0;
}

void ArrayList_Delete(ArrayList* arrayList) {
	if (arrayList->ptr) {
		free(arrayList->ptr);
	}
}