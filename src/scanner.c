#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "include/scanner.h"

typedef struct
{
    const char *start;
    const char *current;
    int line;
} Scanner;

Scanner scanner;

void initScanner(const char *source)
{
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

static bool isAtEnd()
{
    return *scanner.current == '\0';
}

// Advance the character pointer and return the popped character
static char advance()
{
    scanner.current++;
    return scanner.current[-1];
}

static char peek()
{
    return scanner.current[0];
}

static char peekNext()
{
    if (isAtEnd())
        return '\0';
    return scanner.current[1];
}

// Check if next character is 'expected' if so return true and advance character pointer
static bool match(char expected)
{
    if (isAtEnd())
        return false;
    if (*scanner.current != expected)
        return false;
    scanner.current++;
    return true;
}

static Token makeToken(TokenType type)
{
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

static Token errorToken(const char *message)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner.line;
    return token;
}

static void skipWhitespace()
{
    for (;;)
    {
        char c = peek(); // Peek next character, if its skippable, advance and do all over again
        switch (c)
        {
        case ' ':
        case '\r':
        case '\t':
            advance();
            break;
        case '\n':
            scanner.line++;
            advance();
            break;

        case '/':
            if (peekNext() == '/')
            {
                while (peek() != '\n' && !isAtEnd())
                    advance();
            }
            else
            {
                return;
            }
            break;
        default:
            return; // Nothing left to skip over
        }
    }
}

static TokenType checkKeyword(int start, int length, const char *rest, TokenType type)
{
    // memcmp returns 0 if the two memory blocks match bytes
    // Try to match the keyword from start to rest using length and scanner.current & scanner.start
    if (scanner.current - scanner.start == start + length && (memcmp(scanner.start + start, rest, length) == 0))
    {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

// Helper function to create a TOKEN_STRING
static Token string()
{
    while (peek() != '"' && !isAtEnd())
    {
        // Supporting multi-line strings
        if (peek() == '\n')
            scanner.line++;
        advance();
    }

    if (isAtEnd())
        return errorToken("Unterminated string.");

    // We are on the closing quote
    advance();
    return makeToken(TOKEN_STRING);
}

static bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}

// Helper function to create a TOKEN_NUMBER
static Token number()
{
    while (isDigit(peek()))
        advance();

    // Is there a fractional part? consume it
    if (peek() == '.' && isDigit(peekNext()))
    {
        advance();              // consume .
        while (isDigit(peek())) // consume rest of the number
            advance();
    }

    return makeToken(TOKEN_NUMBER);
}

static bool isAlpha(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}

// See: https://en.wikipedia.org/wiki/Trie
// Tries are special cases of the fundamental data structures, Trees and DFA (state machine)
static TokenType identifierType()
{
    switch (scanner.start[0])
    {
    case 'a':
        return checkKeyword(1, 2, "nd", TOKEN_AND);
    case 'c':
        return checkKeyword(1, 4, "lass", TOKEN_CLASS);
    case 'e':
        return checkKeyword(1, 3, "lse", TOKEN_ELSE);
    case 'i':
        return checkKeyword(1, 1, "f", TOKEN_IF);
    case 'n':
        return checkKeyword(1, 2, "il", TOKEN_NIL);
    case 'o':
        return checkKeyword(1, 1, "r", TOKEN_OR);
    case 'p': // this one has branching choices
        if (scanner.current - scanner.start > 1)
        {
            switch (scanner.start[1])
            {
            case 'r': // This one also has branching choices
                if (scanner.current - scanner.start > 2)
                {
                    switch (scanner.start[2])
                    {
                    case 'i':
                        return checkKeyword(3, 2, "nt", TOKEN_PRINT);
                    case 'o':
                        return checkKeyword(3, 1, "c", TOKEN_PROC);
                    }
                }
            }
            break;
        }
        break;
    case 'r':
        return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
    case 's':
        return checkKeyword(1, 4, "uper", TOKEN_SUPER);
    case 'v':
        return checkKeyword(1, 2, "ar", TOKEN_DAT);
    case 'w':
        return checkKeyword(1, 4, "hile", TOKEN_WHILE);
    case 'f': // This one has branching choices
        if (scanner.current - scanner.start > 1)
        {
            switch (scanner.start[1])
            {
            case 'a':
                return checkKeyword(2, 3, "lse", TOKEN_FALSE);
            case 'o':
                return checkKeyword(2, 1, "r", TOKEN_FOR);
            }
        }
        break;
    case 't':
        if (scanner.current - scanner.start > 1)
        {
            switch (scanner.start[1])
            {
            case 'h':
                return checkKeyword(2, 2, "is", TOKEN_THIS);
            case 'r':
                return checkKeyword(2, 2, "ue", TOKEN_TRUE);
            }
        }
    }
    return TOKEN_IDENTIFIER;
}

static Token identifier()
{
    while (isAlpha(peek()) || isDigit(peek()))
        advance();
    return makeToken(identifierType());
}

Token scanToken()
{
    skipWhitespace(); // will advance
    scanner.start = scanner.current;

    if (isAtEnd())
    {
        return makeToken(TOKEN_EOF);
    }

    char c = advance(); // will advance

    if (isAlpha(c))
    {
        return identifier();
    }

    if (isDigit(c))
    {
        return number();
    }

    switch (c)
    {
    case '(':
        return makeToken(TOKEN_LEFT_PAREN);
    case ')':
        return makeToken(TOKEN_RIGHT_PAREN);
    case '{':
        return makeToken(TOKEN_LEFT_BRACE);
    case '}':
        return makeToken(TOKEN_RIGHT_BRACE);
    case ';':
        return makeToken(TOKEN_SEMICOLON);
    case ',':
        return makeToken(TOKEN_COMMA);
    case '.':
        return makeToken(TOKEN_DOT);
    case '-':
        return makeToken(TOKEN_MINUS);
    case '+':
        return makeToken(TOKEN_PLUS);
    case '/':
        return makeToken(TOKEN_SLASH);
    case '*':
        return makeToken(TOKEN_STAR);
    case '!':
        return makeToken(
            match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=':
        return makeToken(
            match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<':
        return makeToken(
            match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '>':
        return makeToken(
            match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    case '"':
        return string();
    }

    return errorToken("Unexpected character");
}
