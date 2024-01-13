#ifndef slorp_value_h
#define slorp_value_h

// This is the VM's notion of a type, not the users
#include <stdbool.h>
typedef enum
{
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
} ValueType;

typedef struct
{
  ValueType type;
  union
  {
    bool boolean;
    double number;
  } as;
} Value;

// Generic operation of comparing two values aka == 
bool valuesEqual(Value a, Value b);

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)

// Demoting slorp values to native C values
#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)

// Promoting native C values to slorp values
#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})

/**
 *  Constnat pool, an array of values.
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
