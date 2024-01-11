#ifndef slorp_common_h
#define slorp_common_h

#include <stdbool.h>

//#define DEBUG_TRACE_EXECUTION
#define DEBUG_PRINT_CODE

#define HANDLE_ERROR(expr, msg, arg)       \
    do                                     \
    {                                      \
        if ((expr))                        \
        {                                  \
            fprintf(stderr, (msg), (arg)); \
            exit(74);                      \
        }                                  \
    } while (false)

#endif
