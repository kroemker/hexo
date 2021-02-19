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
	assert(index >= 0 && index < arrayList->size);
	memcpy((char*)(arrayList->ptr) + index * arrayList->elsize, element, arrayList->elsize);
}

void* ArrayList_Get(ArrayList* arrayList, size_t index) {
	assert(index >= 0 && index < arrayList->size);
	return (void*)((char*)(arrayList->ptr) + index * arrayList->elsize);
}

void* ArrayList_GetFirst(ArrayList* arrayList) {
	if (arrayList->size > 0) {
		return ArrayList_Get(arrayList, 0);
	}
	else {
		return NULL;
	}
}

void* ArrayList_GetLast(ArrayList* arrayList) {
	if (arrayList->size > 0) {
		return ArrayList_Get(arrayList, arrayList->size - 1);
	}
	else {
		return NULL;
	}
}

void ArrayList_AddEmpty(ArrayList* arrayList) {
	arrayList->size++;
	if (arrayList->size >= arrayList->capacity) {
		arrayList->capacity *= 2;
		void* ptr = realloc(arrayList->ptr, arrayList->elsize * arrayList->capacity);
		if (!ptr) {
			assert(0);
		}
		else {
			arrayList->ptr = ptr;
		}
	}
}

void ArrayList_Add(ArrayList* arrayList, void* element) {
	ArrayList_AddEmpty(arrayList);
	ArrayList_Set(arrayList, arrayList->size - 1, element);
}

void ArrayList_Insert(ArrayList* arrayList, size_t index, void* element) {
	if (index == arrayList->size) {
		ArrayList_Add(arrayList, element);
		return;
	}

	assert(index >= 0 && index < arrayList->size);
	ArrayList_AddEmpty(arrayList);
	void* src = (void*)((char*)(arrayList->ptr) + index * arrayList->elsize);
	void* dst = (void*)(((char*)src) + arrayList->elsize);
	memmove(dst, src, (arrayList->size - index) * arrayList->elsize);
	ArrayList_Set(arrayList, index, element);
}

void ArrayList_Remove(ArrayList* arrayList, size_t index) {
	assert(index >= 0 && index < arrayList->size);
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