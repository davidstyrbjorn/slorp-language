#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

void compile(const char *source)
{
    initScanner(source);

    // At any point in time, the compiler needs only one or two tokens
    // So we dont need to keep them all around
    // We do not scan a token until we need one, when compiler needs one we just return next token by value

    int line = -1;
    for (;;)
    {
        Token token = scanToken();
        if (token.line != line)
        {
            printf("%4d ", token.line);
            line = token.line;
        }
        else
        {
            printf("    | ");
        }

        printf("%d '%.*s'\n", token.type, token.length, token.start);

        if (token.type == TOKEN_EOF)
        {
            break;
        }
    }
}