#ifndef slorp_compiler_h
#define slorp_compiler_h

#include<stdbool.h>
#include "chunk.h"

/**
 * @brief Take a users program and fill up the chunk with bytecode
 *
 * @param source text source code
 * @param chunk empty chunk to fill bytecode with
 */
bool compile(const char *source, Chunk *chunk);

#endif
