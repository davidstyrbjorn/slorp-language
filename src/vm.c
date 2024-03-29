#include "include/vm.h"

#include "include/debug.h"
#include "include/value.h"
#include "include/compiler.h"
#include "include/object.h"
#include "include/memory.h"
#include "include/table.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

VM vm; // extern

static void resetStack()
{
    vm.stackTop = vm.stack;
}

static void runtimeError(const char *format, ...)
{
    va_list args; // variadic arguments
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fputs("\n", stderr); // push new line!

    // Positional information
    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack();
}

void initVM()
{
    resetStack();
    vm.objects = NULL;
    initTable(&vm.strings);
    initTable(&vm.globals);
}

void freeVM()
{
    // Free ALL objects
    freeTable(&vm.strings);
    freeTable(&vm.globals);
    freeObjects();
}

static Value peek(int distance)
{
    return vm.stackTop[-1 - distance];
}

static bool isFalsey(Value value)
{
    // Base implementation follows Ruby: nil and false are falsey, every other value is true
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate_strings()
{
    ObjString *b = AS_STRING(pop());
    ObjString *a = AS_STRING(pop());

    int length = a->length + b->length;
    char *chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0'; // Doing this means we can run this char* through functions like printf

    ObjString *result = takeString(chars, length);
    push(OBJ_VAL(result));
}

static InterpretResult run()
{
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(valueType, op)                        \
    do                                                  \
    {                                                   \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) \
        {                                               \
            runtimeError("Operands must be numbers.");  \
            return INTERPRET_RUNTIME_ERROR;             \
        }                                               \
        double b = AS_NUMBER(pop());                    \
        double a = AS_NUMBER(pop());                    \
        push(valueType(a op b));                        \
    } while (false)

    Value constant;
    ObjString *name = NULL;

    for (;;)
    {
#ifdef DEBUG_TRACE_EXECUTION
        printf("       ");
        for (Value *slot = vm.stack; slot < vm.stackTop; slot++)
        {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
        dissassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE())
        {
        case OP_CONSTANT:
            constant = READ_CONSTANT();
            push(constant);
            break;
        case OP_NIL:
            push(NIL_VAL);
            break;
        case OP_TRUE:
            push(BOOL_VAL(true));
            break;
        case OP_FALSE:
            push(BOOL_VAL(false));
            break;
        case OP_POP:
            pop();
            break;
        case OP_GET_LOCAL:
        {
            uint8_t slot = READ_BYTE(); // index that we saved from in the compiliation step
            push(vm.stack[slot]);       // push the index on the stack to the top of the stack
            break;
            break;
        }
        case OP_SET_LOCAL:
        {
            uint8_t slot = READ_BYTE();
            vm.stack[slot] = peek(0);
            break;
        }
        case OP_GET_GLOBAL:
            name = READ_STRING();
            Value value;
            if (!tableGet(&vm.globals, name, &value))
            {
                runtimeError("Undefined variable '%s'", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            push(value); // Two opcodes OP_GET_GLOBAL NAME (index actually) turns into -> VALUE
            break;
        case OP_DEFINE_GLOBAL:
            name = READ_STRING();
            tableSet(&vm.globals, name, peek(0));
            pop();
            break;
        case OP_PRINT:
        {
            printValue(pop()); // The evaluated expression would have left a Value to print top of stack
            printf("\n");
            break;
        }
        case OP_RETURN:
            // Exit interpreter
            return INTERPRET_OK;
        case OP_NEGATE:
            if (!IS_NUMBER(peek(0)))
            {
                runtimeError("Operand must be of type Number");
                return INTERPRET_RUNTIME_ERROR;
            }
            // We know that the top of stack contains a number!
            push(NUMBER_VAL(-AS_NUMBER(pop())));
            break; // Take the top value of the stack, negate it
        case OP_ADD:
        {
            // Concatenation can occur between numbers AND strings
            if (IS_STRING(peek(0)) && IS_STRING(peek(1)))
            {
                concatenate_strings();
            }
            else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1)))
            {
                // Pop our Value's from stack & convert them to C doubles
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                // Perform C addition then convert and push back value on stack
                push(NUMBER_VAL(a + b));
            }
            else
            {
                runtimeError("Operands must be two numbers or two strings.");
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OP_SET_GLOBAL:
            name = READ_STRING();
            if (tableSet(&vm.globals, name, peek(0))) // returns false if it is a new key
            {
                tableDelete(&vm.globals, name);
                runtimeError("Can't assign to undefined variable '%s'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        case OP_EQUAL:
        {
            Value a = pop();
            Value b = pop();
            push(BOOL_VAL(valuesEqual(a, b)));
            break;
        }
        case OP_GREATER:
            BINARY_OP(BOOL_VAL, >);
            break;
        case OP_LESS:
            BINARY_OP(BOOL_VAL, <);
            break;
        case OP_SUBTRACT:
            BINARY_OP(NUMBER_VAL, -);
            break;
        case OP_MULTIPLY:
            BINARY_OP(NUMBER_VAL, *);
            break;
        case OP_DIVIDE:
            BINARY_OP(NUMBER_VAL, /);
            break;
        case OP_NOT:
            push(BOOL_VAL(isFalsey(pop())));
            break;
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
#undef READ_STRING
}

InterpretResult interpret(const char *source)
{
    Chunk chunk;
    initChunk(&chunk);

    if (!compile(source, &chunk))
    {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    InterpretResult result = run();

    freeChunk(&chunk);
    return result;
}

void push(Value value)
{
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop()
{
    vm.stackTop--;
    return *vm.stackTop;
}
