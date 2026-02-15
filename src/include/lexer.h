#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef enum TokenType  {
    TK_KW_AND, TK_KW_ANS, TK_KW_DEF, TK_KW_OF, TK_KW_OR, TK_KW_NOT,
    TK_KW_TRUE, TK_KW_FALSE, TK_KW_IF, TK_KW_ELSE,
    TK_ADD, TK_SUB, TK_DIV, TK_MUL, TK_MOD,
    TK_KW_LET,
    TK_ARROW, TK_AT_THE_RATE, TK_DOLLAR,
    TK_EXCLAIM, TK_CARET, TK_INT_DIV, TK_EQUAL,
    TK_IDENTIFIER, TK_NUMBER, TK_UNDERSCORE, TK_COMMA,
    TK_OPEN_PAREN, TK_CLOSE_PAREN,
    TK_GT, TK_GT_EQ, TK_SM, TK_SM_EQ,
    TK_EQUALITY, TK_NOT_EQUAL,
    TK_PIPE, TK_SEMICOLON, TK_EOL
} TokenType;

typedef struct Token {
    enum TokenType type;
    unsigned int start, len;
} Token;

void initLexer();
void freeLexer();
Token* tokenize(const char* str, size_t len, size_t *outLen);

// the returned pointer is overriden after next call to the function
// the retuened pointer is NOT heap allocated
char* tkToString(const Token*);

// use only for TK_IDENTIFIER, the returned pointer is NOT heap allocated
char* getIdentifier(const Token tk);

bool tkToNumber(const Token*, double* outNum);
void display();
