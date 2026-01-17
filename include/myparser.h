#ifndef __PARSER_H__
#define __PARSER_H__

#include "ast.h"
#include "lexer.h"

typedef enum {
    STMT_SELECT,
    STMT_INSERT,
    STMT_CREATE,
    STMT_DELETE,
    STMT_UPDATE
} StatementType;

typedef struct _Statement {
    StatementType type;
} Statement;

typedef struct _Parser {
    Lexer *lexer;
    Token current;
} Parser;

void parser_init(Parser *, Lexer *);
ASTNode *parse_statement(Parser *);
void free_statement(Statement *);

#endif /* __PARSER_H__ */