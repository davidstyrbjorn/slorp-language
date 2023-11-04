#ifndef slorp_chunk_h
#define slorp_chunk_h

#include "common.h"
#include "value.h"

/**
 * @brief Our bytecode format
 * each instruction has one byte of operation code (opcode)
 * that number controls what kind of operation we're dealing with -
 * add, subtract, call, etc
 * this is their definition
 */
typedef enum
{
    OP_CONSTANT,
    OP_RETURN,
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