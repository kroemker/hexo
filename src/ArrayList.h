#ifndef ARRAYLIST_H
#define ARRAYLIST_H

#include <stdlib.h>

typedef struct {
	void* ptr;
	size_t capacity;
	size_t size;
	size_t elsize;
}ArrayList;

ArrayList ArrayList_New(size_t initialCapacity, size_t elementSize);

ArrayList ArrayList_NewFromArray(void* array, size_t size, size_t elementSize);

void ArrayList_Set(ArrayList* arrayList, size_t index, void* element);

void* ArrayList_Get(ArrayList* arrayList, size_t index);

void* ArrayList_GetFirst(ArrayList* arrayList);

void* ArrayList_GetLast(ArrayList* arrayList);

void ArrayList_AddEmpty(ArrayList* arrayList);

void ArrayList_Add(ArrayList* arrayList, void* element);

void ArrayList_Insert(ArrayList* arrayList, size_t index, void* element);

void ArrayList_Remove(ArrayList* arrayList, size_t index);

void ArrayList_Clear(ArrayList* arrayList);

void ArrayList_Delete(ArrayList* arrayList);

#endif