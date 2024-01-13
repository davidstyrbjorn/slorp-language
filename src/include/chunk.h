#ifndef slorp_chunk_h
#define slorp_chunk_h

#include "value.h"
#include <stdint.h>

/**
 * @brief Our bytecode format
 * each instruction has one byte of operation code (opcode)
 * that number controls what kind of operation we're dealing with -
 * add, subtract, call, etc
 * this is their definition
 */
typedef enum
{
    OP_CONSTANT, // 1, 2, 3
    OP_NEGATE, // -something
    OP_RETURN, // return something
    OP_NIL, // nil
    OP_TRUE, // true
    OP_FALSE, // false
    OP_NOT, // !something
    OP_ADD, // a + b
    OP_SUBTRACT, // a - b
    OP_MULTIPLY, // a * b
    OP_DIVIDE, // a / b
    OP_EQUAL, // ==
    OP_GREATER, // >
    OP_LESS, // <    
} OpCode;

/**
 * @brief Bytecode is a series of instructions.
 *
 */
typedef struct
{
    int count;
    int capacity;
    uint8_t *code;
    int *lines;
    ValueArray constants;
} Chunk;

void initChunk(Chunk *chunk);
void writeChunk(Chunk *chunk, uint8_t byte, int line);
void freeChunk(Chunk *chunk);
int addConstant(Chunk *chunk, Value value); // Convenience function to add constants into a chunk

#endif
