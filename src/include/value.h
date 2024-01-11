#ifndef slorp_value_h
#define slorp_value_h

#include "common.h"

typedef double Value;

/**
 * @brief Constnat pool, an array of values.
 * An instrction will look up a value in the array by index
 */
typedef struct
{
    int capacity;
    int count;
    Value *values;
} ValueArray;

void initValueArray(ValueArray *array);
void writeValueArray(ValueArray *array, Value value);
void freeValueArray(ValueArray *array);
void printValue(Value value); // Utility for printing a Slorp Value

#endif
