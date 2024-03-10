#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "include/compiler.h"
#include "include/chunk.h"
#include "include/scanner.h"
#include "include/value.h"
#include "include/object.h"
#include "include/error.h"

#ifdef DEBUG_PRINT_CODE
#include "include/debug.h"
#endif

// This is a Pratt parser

// For variable declerations three operations are needed
// - Declaring a new variable using a dat statement
// - Accessing the value of a variable using an identifier expression

/*
 * advance(), errorAtCurrent(), error(), consume() emitByte emitBytes()
 */

// Executing a block simply means executing the statement it contains, one after the other

// This define's Slorps precedence level in order from lowest to highest.
typedef enum
{
    PREC_NONE = 0,
    PREC_ASSIGNMENT, // =
    PREC_OR,         // or
    PREC_AND,        // and
    PREC_EQUALITY,   // == !=
    PREC_COMPARISON, // < > <= >=
    PREC_TERM,       // + -
    PREC_FACTOR,     // * /
    PREC_UNARY,      // ! -
    PREC_CALL,       // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct
{
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

typedef struct
{
    Token name;
    int depth;
} Local;

typedef struct
{
    Local locals[UINT8_COUNT];
    int localCount; // traks how many array slots are in use in 'locals'
    int scopeDepth; // number of blocks surrounding the current bit of code we're compiling
} Compiler;

Parser parser = {
    .hadError = false,
    .panicMode = false,
};
Chunk *compilingChunk;
Compiler *current = NULL;

static Chunk *currentChunk()
{
    return compilingChunk;
}

/// @brief Function for stepping through a token stream, front end of the parser/compiler
static void advance()
{
    parser.previous = parser.current;

    for (;;)
    {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR)
            break;

        errorAtCurrentToken(parser.current.start);
    }
}

/// @brief Similar to check and match but does both thing in one function TODO: not needed?
static void consume(TokenType type, const char *message)
{
    if (parser.current.type == type)
    {
        advance();
        return;
    }

    errorAtCurrentToken(message);
}

/// @brief Thin helper for checking if `type` is the currently parsed token
static bool check(TokenType type)
{
    return parser.current.type == type;
}

/// @brief check if `type` is the currently parsed token,
///        if it is, advance passed it and return true, otherwise return false
static bool match(TokenType type)
{
    if (!check(type))
        return false;
    advance();
    return true;
}

// "emitByte" write byte to the currentChunk aka our compilingChunk
static void emitByte(uint8_t byte)
{
    writeChunk(currentChunk(), byte, parser.previous.line);
}

// Helper to emit 2 bytes with one function call instead of two
static void emitBytes(uint8_t byte1, uint8_t byte2)
{
    emitByte(byte1);
    emitByte(byte2);
}

static void emitReturn()
{
    emitByte(OP_RETURN);
}

// Add a value constant to the currentChunk()
static uint8_t makeConstant(Value value)
{
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX)
    {
        errorAtPreviousToken("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant; // the index of the place the number is stored on the stack
}

static void emitConstant(Value value)
{
    emitBytes(OP_CONSTANT, makeConstant(value));
}

static void initCompiler(Compiler *compiler)
{
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    current = compiler;
}

static void endCompiler()
{
    emitReturn();

#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError)
    {
        dissassembleChunk(currentChunk(), "code");
    }
    else
    {
        printf("Error");
    }
#endif
}

static void beginScope()
{
    current->scopeDepth++;
}

static void endScope()
{
    current->scopeDepth--;

    // Remove all locals from the ended scope
    while (current->localCount > 0 &&
           current->locals[current->localCount - 1].depth > current->scopeDepth)
    {
        current->localCount--;
        emitByte(OP_POP); // OP_POPN?
    }
}

static void expression();
static void statement();
static void decleration();
static ParseRule *getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static void grouping(bool canAssign)
{
    expression(); // Recursive as heck
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

/*
 * To compile number literals we store a pointer to the following function at the TOKEN_NUMBE index in the array
 */

static void number(bool canAssign)
{
    double value = strtold(parser.previous.start, NULL); // Converting char* to double here
    emitConstant(NUMBER_VAL(value));
}

static void string(bool canAssign)
{
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1,
                                    parser.previous.length - 2)));
}

static uint8_t identifierConstant(Token *name)
{
    return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

static bool identifierEqual(Token *a, Token *b)
{
    if (a->length != b->length)
        return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

// Given name of a local variable, returns the index of its position in the locals array, -1 if not found
static int resolveLocal(Compiler *compiler, Token *name)
{
    for (int i = compiler->localCount - 1; i >= 0; i--)
    {
        Local *l = &compiler->locals[i];
        if (identifierEqual(name, &l->name))
        {
            if (l->depth == -1)
            {
                errorAtCurrentToken("Can't resolve local variable in its own initalizer.");
            }
            return i;
        }
    }

    return -1;
}

static void addLocal(Token name)
{
    if (current->localCount >= UINT8_COUNT)
    {
        errorAtCurrentToken("Too many local variables in block.");
        return;
    }
    Local *local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
}

static void declareVariable()
{
    if (current->scopeDepth == 0)
        return;
    Token *name = &parser.previous;
    for (int i = current->localCount - 1; i >= 0; i--)
    {
        Local *otherLocal = &current->locals[i];
        if (otherLocal->depth != -1 && otherLocal->depth < current->scopeDepth)
        {
            break;
        }
        if (identifierEqual(name, &otherLocal->name))
        {
            errorAtCurrentToken("Already a variable with this name in scope.");
        }
    }
    addLocal(*name);
}

static void namedVariable(Token name, bool canAssign)
{
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, &name);
    if (arg != -1) // We have a local
    {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    }
    else
    {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    if (canAssign && match(TOKEN_EQUAL))
    {
        expression(); // Evaluate the whole expression to the right of the equal
        emitBytes(setOp, (uint8_t)arg);
    }
    else
    {
        emitBytes(getOp, (uint8_t)arg);
    }
}

static void variable(bool canAssign)
{
    namedVariable(parser.previous, canAssign);
}

static void parsePrecedence(Precedence precedence)
{
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL)
    {
        errorAtCurrentToken("Expected expression");
        return;
    }

    // Is the left side (prefix) an assignable target?
    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence)
    {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    if (canAssign && match(TOKEN_EQUAL))
    {
        errorAtCurrentToken("Invalid assignment target");
    }
}

static uint8_t parseVariable(const char *errorMessage)
{
    // Requires the current token to be identifier, consume it
    consume(TOKEN_IDENTIFIER, errorMessage);

    declareVariable();
    if (current->scopeDepth > 0)
        return 0;

    return identifierConstant(&parser.previous); // from the previous token, make a constant and return its index
}

static void markInitalized()
{
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable(uint8_t global)
{
    if (current->scopeDepth > 0) // not in global scope?
    {
        markInitalized();
        return;
    }
    // Variable is stored in bytecode as as a OP_DEFINE_GLOBAL variable byte followed by the lookup index into the global variable table
    emitBytes(OP_DEFINE_GLOBAL, global);
}

static void unary(bool canAssign)
{
    // Might seem weird to do expression and THEN emit the OP_NEGATE opcode but
    // it is part of the compilers job to take the source code (stream of tokens)
    // and re-arrange it to the correct execution order
    // Here, when we call expression it will leave the evaluate and put on top of stack
    // Then we pop that value, negate it and push the result.

    TokenType operatorType = parser.previous.type;

    // Evaluate expression as UNARY precedence
    parsePrecedence(PREC_UNARY);

    // Emit the operator instruction
    switch (operatorType)
    {
    case TOKEN_BANG:
        emitByte(OP_NOT);
        break;
    case TOKEN_MINUS:
        emitByte(OP_NEGATE);
        break;
    default:
        return; // Unreachable (syntatically)
    }
}

// Prefix parser function, leading token (constant) has already been consumed
static void binary(bool canAssign)
{
    TokenType operatorType = parser.previous.type;
    // This shall now consume the other part of the binary operation b of a op b
    ParseRule *rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType)
    {
    case TOKEN_BANG_EQUAL:
        emitBytes(OP_EQUAL, OP_NOT);
        break;
        break;
    case TOKEN_EQUAL_EQUAL:
        emitByte(OP_EQUAL);
        break;
    case TOKEN_GREATER:
        emitByte(OP_GREATER);
        break;
    case TOKEN_GREATER_EQUAL:
        emitBytes(OP_LESS, OP_NOT);
        break;
    case TOKEN_LESS:
        emitByte(OP_LESS);
        break;
    case TOKEN_LESS_EQUAL:
        emitBytes(OP_GREATER, OP_NOT);
        break;
    case TOKEN_PLUS:
        emitByte(OP_ADD);
        break;
    case TOKEN_MINUS:
        emitByte(OP_SUBTRACT);
        break;
    case TOKEN_STAR:
        emitByte(OP_MULTIPLY);
        break;
    case TOKEN_SLASH:
        emitByte(OP_DIVIDE);
        break;
    default:
        return; // unreachable
    }
}

static void literal(bool canAssign)
{
    switch (parser.previous.type)
    {
    case TOKEN_FALSE:
        emitByte(OP_FALSE);
        break;
    case TOKEN_NIL:
        emitByte(OP_NIL);
        break;
    case TOKEN_TRUE:
        emitByte(OP_TRUE);
        break;
    default:
        return; // unreachable lol
    }
}

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, NULL, PREC_NONE},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_PROC] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, NULL, PREC_NONE},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
    [TOKEN_DAT] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

static ParseRule *getRule(TokenType type)
{
    return &rules[type];
}

static void expression()
{
    parsePrecedence(PREC_ASSIGNMENT); // we call into parsing with the lowest precedence level
}

static void block()
{
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
    {
        decleration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expecteed '}' after block statement.");
}

static void varDecleration()
{
    // Consumes identifier token for the var name
    // adds it lexeme to the chunk's constant table as a string
    // and then returns the constant table index where it was added
    uint8_t globalIndex = parseVariable("Expect variable name.");

    if (match(TOKEN_EQUAL))
    {
        expression(); // This should emit the actuallu initalization value
    }
    else // desugars var a; into var a = nil;
    {
        emitByte(OP_NIL);
    }

    // emit the bytecode for storing the variable's value in the global variables hash table
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration");
    defineVariable(globalIndex);
}

static void expressionStatement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
}

static void printStatement()
{
    expression(); // Evaluate an expression then emit a OP_PRINT
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT); // The expression should have produced some sort of Value on the stack we can print!
}

static void synchronize()
{
    parser.panicMode = false;
    while (parser.current.type != TOKEN_EOF)
    {
        // We indescrimentally skip tokens until we reach something that looks like the end of a statement (semicolon)
        // Or until we reach something that looks like the beggining of a new statement. Usually a variable decleration or control flow keyword
        if (parser.previous.type == TOKEN_SEMICOLON)
            return;
        switch (parser.current.type)
        {
        case TOKEN_CLASS:
        case TOKEN_PROC:
        case TOKEN_DAT:
        case TOKEN_FOR:
        case TOKEN_IF:
        case TOKEN_WHILE:
        case TOKEN_PRINT:
        case TOKEN_RETURN:
            return;
        default:; // Do nohting
        }

        advance();
    }
}

static void decleration()
{
    if (match(TOKEN_DAT))
    {
        varDecleration();
    }
    else
    {
        statement();
    }

    if (parser.panicMode)
        synchronize();
}

static void statement()
{
    // Statement is either a print or a decleration
    if (match(TOKEN_PRINT))
    {
        printStatement();
    }
    else if (match(TOKEN_LEFT_BRACE))
    {
        beginScope(); // {
        block();      // ...kråd..
        endScope();   // }
    }
    else
    {
        expressionStatement();
    }
}

bool compile(const char *source, Chunk *chunk)
{
    initScanner(source);
    Compiler compiler;
    initCompiler(&compiler);
    compilingChunk = chunk; // Pointer to the chunk of bytecode we are compiling TO

    advance();

    // -> this wwas when we did only 1 expression! expression();
    while (!match(TOKEN_EOF)) // a program is a sequence of declerations
    {
        decleration();
    }

    consume(TOKEN_EOF, "Expect end of expression.");

    endCompiler();
    return !parser.hadError;
}
