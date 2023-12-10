/*
 * sparse_array.c
 * @brief
 * Created on: Feb 22, 2021
 * Author: Yanye
 */

#include "utils/sparse_array.h"

static BOOL ICACHE_FLASH_ATTR put(SparseArray *self, size_t key, size_t value);
static BOOL ICACHE_FLASH_ATTR get(SparseArray *self, size_t key, size_t *value);
static void ICACHE_FLASH_ATTR clear(SparseArray *self);
static void ICACHE_FLASH_ATTR delete(SparseArray **self);

SparseArray * ICACHE_FLASH_ATTR newSparseArray(uint32_t initialCapacity) {
    SparseArray *array = (SparseArray *)os_malloc(sizeof(SparseArray));
    if(array != NULL) {
        array->position = 0;
        array->capacity = initialCapacity;

        array->keys = (size_t *)os_malloc(sizeof(size_t) * initialCapacity);
        array->values = (size_t *)os_malloc(sizeof(size_t) * initialCapacity);
        array->put = put;
        array->get = get;
        array->delete = delete;
        array->clear = clear;
    }
    return array;
}

static BOOL ICACHE_FLASH_ATTR put(SparseArray *self, size_t key, size_t value) {
    if(self->position > (self->capacity - 1)) {
        return FALSE;
    }
    *(self->keys + self->position) = key;
    *(self->values + self->position) = value;
    self->position++;
    return TRUE;
}

static BOOL ICACHE_FLASH_ATTR get(SparseArray *self, size_t key, size_t *value) {
	uint32_t i = 0;
    for(; i < self->position; i++) {
        if(*(self->keys + i) == key) {
            *value = *(self->values + i);
            return TRUE;
        }
    }
    return FALSE;
}

static void ICACHE_FLASH_ATTR clear(SparseArray *self) {
	self->position = 0;
}

static void ICACHE_FLASH_ATTR delete(SparseArray **self) {
    SparseArray *array = *self;
    if(array != NULL) {
        os_free(array->keys);
        os_free(array->values);
        os_free(array);
        *self = NULL;
    }
}


