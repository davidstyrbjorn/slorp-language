#ifndef slorp_memory_h
#define slorp_memory_h

#include"object.h"

#include <stddef.h>

#define INITAL_DYNAMIC_ARRAY_SIZE 8
#define GROW_CAPACITY(capacity) \
    ((capacity) < INITAL_DYNAMIC_ARRAY_SIZE ? INITAL_DYNAMIC_ARRAY_SIZE : (capacity)*2)

#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type *)reallocate(pointer, sizeof(type) * (oldCount), sizeof(type) * (newCount))

#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, sizeof(type) * (oldCount), 0)

#define ALLOCATE(type, count) \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

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

// Looks at the VM's globaly allocated objects and frees all
void freeObjects();

#endif
