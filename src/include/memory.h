#ifndef slorp_memory_h
#define slorp_memory_h

#include "common.h"

#define INITAL_DYNAMIC_ARRAY_SIZE 8
#define GROW_CAPACITY(capacity) \
    ((capacity) < INITAL_DYNAMIC_ARRAY_SIZE ? INITAL_DYNAMIC_ARRAY_SIZE : (capacity)*2)

#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type *)reallocate(pointer, sizeof(type) * (oldCount), sizeof(type) * (newCount))

#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, sizeof(type) * (oldCount), 0)

/**
 * @brief
 *
 * @param pointer pointing to the start of the memory block we want reallocate
 * @param oldSize
 * @param newSize
 * @return the new block of memory
 * @brief We either, allocate new block, free allocation, shrink existing allocation or grow existing allocation
 */
void *reallocate(void *pointer, size_t oldSize, size_t newSize);

#endif