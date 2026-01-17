#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "lexer.h"
#include "utils.h"

static int is_keyword(const char *s, TokenType *out)
{
    struct { const char *k; TokenType t; } keywords[] = {
        {"CREATE", TOK_CREATE}, {"TABLE", TOK_TABLE},
        {"INSERT", TOK_INSERT}, {"INTO", TOK_INTO},
        {"VALUES", TOK_VALUES}, {"SELECT", TOK_SELECT}, {"FROM", TOK_FROM}, 
        {"JOIN", TOK_JOIN}, {"ON", TOK_ON}, {"WHERE", TOK_WHERE},
        {"DELETE", TOK_DELETE}, {"UPDATE", TOK_UPDATE},
        {"SET", TOK_SET},
        {"PRIMARY", TOK_PRIMARY}, {"KEY", TOK_KEY},
        {"UNIQUE", TOK_UNIQUE},
        {"INT", TOK_INT}, {"TEXT", TOK_TEXT}, {"BOOL", TOK_BOOL},
        {"TRUE", TOK_TRUE}, {"FALSE", TOK_FALSE},
        {NULL, 0}
    };
    for(int i = 0; keywords[i].k; i++)
    {
        if(strcmp(s, keywords[i].k) == 0)
        {
            *out = keywords[i].t;
            return 1;
        }
    }
    return 0;
}


void lexer_init(Lexer *lx, const char *input)
{
    lx->input = input;
    lx->pos = 0;
}

Token lexer_next(Lexer *lx)
{
    Token tok = {TOK_EOF, NULL};

    while(isspace(lx->input[lx->pos])) lx->pos++;

    char c = lx->input[lx->pos];
    if(!c) return tok;

    if(isalpha(c))
    {
        int start = lx->pos;
        while(isalnum(lx->input[lx->pos]) || lx->input[lx->pos] == '_')
            lx->pos++;
        
        int len = lx->pos - start;
        char *word = strndup(lx->input + start, len);

        TokenType type;
        if(is_keyword(word, &type))
        {
            free(word);
            tok.type = type;
        }
        else
        {
            tok.type = TOK_IDENTIFIER;
            tok.lexme = word;
        }
        return tok;
    }

    if(isdigit(c))
    {
        int start = lx->pos;
        while(isdigit(lx->input[lx->pos])) lx->pos++;
        tok.type = TOK_NUMBER;
        tok.lexme = strndup(lx->input + start, lx->pos - start);
        return tok;
    }

    if(c == '"')
    {
        lx->pos++;
        int start = lx->pos;
        while(lx->input[lx->pos] && lx->input[lx->pos] != '"')
            lx->pos++;
        tok.type = TOK_STRING;
        tok.lexme = strndup(lx->input + start, lx->pos - start);
        lx->pos++;
        return tok;
    }

    lx->pos++;
    switch(c)
    {
        case '.': tok.type = TOK_DOT; break;
        case '*': tok.type = TOK_STAR; break;
        case ',': tok.type = TOK_COMMA; break;
        case '(': tok.type = TOK_LPAREN; break;
        case ')': tok.type = TOK_RPAREN; break;
        case '=': tok.type = TOK_EQUAL; break;
        case ';': tok.type = TOK_SEMICOLON; break;
        default: tok.type = TOK_EOF; break;
    }

    return tok;
}
