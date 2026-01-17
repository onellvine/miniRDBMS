#ifndef __TOKEN_H__
#define __TOKEN_H__

#include <stdlib.h>

typedef enum {
    TOK_EOF = 0,

    TOK_IDENTIFIER,
    TOK_NUMBER,
    TOK_STRING,

    TOK_CREATE, TOK_TABLE,
    TOK_INSERT, TOK_INTO, TOK_VALUES,
    TOK_SELECT, TOK_FROM, TOK_JOIN, TOK_ON, TOK_WHERE,
    TOK_DELETE, TOK_UPDATE, TOK_SET,

    TOK_PRIMARY, TOK_KEY, TOK_UNIQUE,
    TOK_INT, TOK_TEXT, TOK_BOOL,
    TOK_TRUE, TOK_FALSE,

    TOK_DOT,
    TOK_STAR,
    TOK_COMMA,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_EQUAL,
    TOK_SEMICOLON
} TokenType;

typedef struct _Token {
    TokenType type;
    char *lexme;
} Token;

static inline void token_free(Token *t)
{
    free(t->lexme);
    t->lexme = NULL;
}

#endif /* __TOKEN_H__ */
