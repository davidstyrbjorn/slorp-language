#include "include/error.h"

#include "include/scanner.h"
#include "include/compiler.h"

#include <stdio.h>

static void errorAt(Token *token, const char *message)
{
    if (parser.panicMode)
        return;
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF)
    {
        fprintf(stderr, " at end");
    }
    else if (token->type == TOKEN_ERROR)
    {
        // Nothing.
    }
    else
    {
        fprintf(stderr, " at '%.*s' ", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

void errorAtCurrentToken(const char *message)
{
    errorAt(&parser.current, message);
}

void errorAtPreviousToken(const char *message)
{
    errorAt(&parser.previous, message);
}
