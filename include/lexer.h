#ifndef __LEXER_H__
#define __LEXER_H__


#include "token.h"

typedef struct _Lexer {
    const char *input;
    int pos;
} Lexer;

void lexer_init(Lexer *, const char *);
Token lexer_next(Lexer *);

#endif /* __LEXER_H__ */