/*
 * sparse_array.h
 * @brief
 * Created on: Feb 22, 2021
 * Author: Yanye
 */

#ifndef APP_INCLUDE_UTILS_SPARSE_ARRAY_H_
#define APP_INCLUDE_UTILS_SPARSE_ARRAY_H_

#include "c_types.h"
#include "mem.h"

struct _sparse_array;
typedef struct _sparse_array    SparseArray;

struct _sparse_array{
    size_t *keys;
    size_t *values;
    uint32_t position;
    uint32_t capacity;
    BOOL (*put)(SparseArray *self, size_t key, size_t value);
    BOOL (*get)(SparseArray *self, size_t key, size_t *value);
    void (*clear)(SparseArray *self);
    void (*delete)(SparseArray **self);
};

SparseArray *newSparseArray(uint32_t initialCapacity);


#endif /* APP_INCLUDE_UTILS_SPARSE_ARRAY_H_ */
