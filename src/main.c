#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "include/common.h"
#include "include/vm.h"
#include "include/table.h"
#include "include/object.h"

#define DUMMY_LINE 123

/**
 * @brief  scanner -> compiler -> VM
 * Source code goes into scanner
 * Token comes from scanner into compiler
 * Bytecode chunk goes into VM and is ran
 *
 */

/**
 * @brief
     statement      → exprStmt
                    | printStmt ;

    * declaration    → varDecl
                    | statement ;
*/

/**
 * @brief Take input from user while input is valid (ctrl+c will break)
 * run each line through interpret
 */
static void repl()
{
    char line[1024];
    for (;;)
    {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin))
        {
            printf("\n");
            break;
        }

        interpret(line);
    }
}

/**
 * @brief Takes file path returns allocated char* string of the files text content
 *
 */
static char *readFile(const char *path)
{
    FILE *file = fopen(path, "rb");
    HANDLE_ERROR(file == NULL, "Could not open file \"%s\".\n", path);

    fseek(file, 0L, SEEK_END);           // move file to end
    const size_t fileSize = ftell(file); // tell the size when stream is at end
    rewind(file);                        // point back to start of stream

    char *buffer = (char *)malloc(fileSize + 1);
    HANDLE_ERROR(buffer == NULL, "Not enough memory to read \"%s\".\n", path);
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    HANDLE_ERROR(bytesRead < fileSize, "Could not read file \"%s\".\n", path);
    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

/**
 * @brief Reads file from path into string then runs it through interpreter
 * @param path to file
 */
static void runFile(const char *path)
{
    char *source = readFile(path);
    // char *source = "";
    InterpretResult result = interpret(source);
    free(source);

    switch (result)
    {
    case INTERPRET_COMPILE_ERROR:
        exit(65);
        break;
    case INTERPRET_RUNTIME_ERROR:
        exit(70);
        break;
    default:
        break;
    }
}

int main(int argc, const char *argv[])
{
    initVM();

    runFile("test.slorp");

    // if (argc == 1)
    // {
    //     repl();
    // }
    // else if (argc == 2)
    // {
    //     runFile(argv[1]);
    // }
    // else
    // {
    //     fprintf(stderr, "Usage: clox [path]\n");
    // }

    freeVM();

    return 0;
}
