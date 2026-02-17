#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "myparser.h"
#include "utils.h"

static void error(const char *msg)
{
    fprintf(stderr, "Parse error: %s\n", msg);

    exit(EXIT_FAILURE);
}

static void advance(Parser *p)
{
    if(p->current.lexme)
        free(p->current.lexme);
    p->current = lexer_next(p->lexer);
    // return p->current;
}

static void expect(Parser *p, TokenType t, const char *msg)
{
    if(p->current.type != t)
        error(msg);
    // printf("Parsed %d\n", p->current.type);
    advance(p);
}


void parser_init(Parser *p, Lexer *lx)
{
    p->lexer = lx;
    p->current = lexer_next(lx);
}

static Expr *parse_expr(Parser *p)
{
    Expr *e = calloc(1, sizeof(Expr));

    if(p->current.type == TOK_STRING ||
       p->current.type == TOK_NUMBER ||
       p->current.type == TOK_TRUE || 
       p->current.type == TOK_FALSE) {

        e->type = EXPR_LITERAL;
        e->value = strdup(p->current.lexme);
        advance(p);

        return e;
    }

    error("Expected literal expression");

    return NULL;
}

static ColumnType parse_type(Parser *p)
{
    if(p->current.type == TOK_INT)
    {
        advance(p);
        return COL_INT;
    }
    if(p->current.type == TOK_TEXT)
    {
        advance(p);
        return COL_TEXT;
    }
    if(p->current.type == TOK_BOOL)
    {
        advance(p);
        return COL_BOOL;
    }

    error("Unknown column type");

    return 0;
}

static ASTNode *parse_create(Parser *p)
{
    expect(p, TOK_TABLE, "Expected TABLE");

    if(p->current.type != TOK_IDENTIFIER)
        error("Expected table name");

    ASTNode *n = calloc(1, sizeof(ASTNode));
    n->type = AST_CREATE;
    n->create.table_name = strdup(p->current.lexme);
    advance(p);

    expect(p, TOK_LPAREN, "Expected '('");

    while (1)
    {
        ColumnDef col = {0};

        if(p->current.type != TOK_IDENTIFIER)
            error("Expected column name");
        
        col.name = strdup(p->current.lexme);
        advance(p);

        col.type = parse_type(p);

        if(p->current.type == TOK_PRIMARY)
        {
            advance(p);
            expect(p, TOK_KEY, "Expected KEY");
            col.primary_key = 1;
        }
        if(p->current.type == TOK_UNIQUE)
        {
            advance(p);
            col.unique = 1;
        }

        n->create.columns = realloc(
            n->create.columns,
            sizeof(ColumnDef) * (n->create.column_count + 1)
        );
        n->create.columns[n->create.column_count++] = col;

        if(p->current.type != TOK_COMMA)
            break;
        advance(p);
    }
    
    expect(p, TOK_RPAREN, "Expected ')'");
    // printf("Create with columns: %d\n", n->create.column_count);

    return n;
}

static Expr *parse_literal(Parser *p)
{
    Expr *e = calloc(1, sizeof(Expr));
    e->type = EXPR_LITERAL;
    e->value = strdup(p->current.lexme);

    advance(p);

    return e;
}

static ASTNode *parse_insert(Parser *p)
{
    expect(p, TOK_INTO, "Expected INTO");

    ASTNode *n = calloc(1, sizeof(ASTNode));
    n->type = AST_INSERT;

    if(p->current.type != TOK_IDENTIFIER)
        error("Expected table name");

    n->insert.table_name = strdup(p->current.lexme);
    advance(p);
    
    expect(p, TOK_VALUES, "Expected VALUES");
    expect(p, TOK_LPAREN, "Expected '('");

    while(1)
    {
        if(p->current.type != TOK_STRING &&
            p->current.type != TOK_NUMBER &&
            p->current.type != TOK_TRUE && 
            p->current.type != TOK_FALSE)
        {
            error("Expected literal");
        }
        n->insert.row.values = realloc(
            n->insert.row.values,
            sizeof(Expr *) * (n->insert.row.value_count + 1)
        );
        n->insert.row.values[n->insert.row.value_count++] = parse_literal(p);

        if(p->current.type != TOK_COMMA)
            break;
        advance(p);
    }

    expect(p, TOK_RPAREN, "Expected ')'");

    return n;
}

static JoinClause *parse_join(Parser *p)
{
    expect(p, TOK_JOIN, "Expected JOIN clause");

    if(p->current.type != TOK_IDENTIFIER)
        error("Expected right table name in JOIN clause");    
    
    JoinClause *j = calloc(1, sizeof(JoinClause));
    j->right_table = strdup(p->current.lexme);
    advance(p);

    expect(p, TOK_ON, "Expected ON");

    if(p->current.type != TOK_IDENTIFIER)
        error("Expected left table name in JOIN clause");
    advance(p);

    expect(p, TOK_DOT, "Expected DOT");

    if(p->current.type != TOK_IDENTIFIER)
        error("Expected left table column name in JOIN clause");
    j->left_column = strdup(p->current.lexme);
    advance(p);

    expect(p, TOK_EQUAL, "Expected EQUAL");

    if(p->current.type != TOK_IDENTIFIER)
        error("Expected right table name in JOIN clause");
    advance(p);
    
    expect(p, TOK_DOT, "Expected DOT");

    if(p->current.type != TOK_IDENTIFIER)
        error("Expected right table column name in JOIN clause");
    j->right_column = strdup(p->current.lexme);    

    return j;
}

static WhereClause *parse_where(Parser *p)
{
    expect(p, TOK_WHERE, "Expected WHERE clause");

    if(p->current.type != TOK_IDENTIFIER)
        error("Expected column name in WHERE clause");
    
    WhereClause *w = calloc(1, sizeof(WhereClause));
    w->coulmn = strdup(p->current.lexme);
    // printf("WHERE %s = ", w->coulmn);
    advance(p);

    expect(p, TOK_EQUAL, "Expected '=' in WHERE clause");

    w->value = parse_expr(p);
    // printf("%s\n", w->value->value);

    return w;
}

static void parse_column_list(Parser *p, SelectStmt *s)
{
    if(p->current.type == TOK_STAR)
    {
        s->select_all = 1;
        advance(p);

        return; // to the caller
    }

    s->select_all = 0;
    s->columns = NULL;
    s->column_count = 0;

    while(1)
    {
        if(p->current.type != TOK_IDENTIFIER)
            error("Expected column name");
        
        s->columns = realloc(
            s->columns,
            sizeof(char *) * (s->column_count + 1)
        );
        s->columns[s->column_count++] = strdup(p->current.lexme);
        advance(p);

        if(p->current.type != TOK_COMMA)
            break;
        advance(p);
    }
    // printf("Queried columns: %d\n", s->column_count);
    // for(int i = 0; i < s->column_count; i++)
    //     printf("%s ", s->columns[i]);
    // printf("\n");
}

static ASTNode *parse_select(Parser *p)
{
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = AST_SELECT;

    parse_column_list(p, &node->select);

    expect(p, TOK_FROM, "Expected FROM");

    if(p->current.type != TOK_IDENTIFIER)
        error("Expected table name");
    
    node->select.table = strdup(p->current.lexme);
    advance(p);

    // Case select statement has a JOIN clause
    if(p->current.type == TOK_JOIN)
    {
        // printf("Select has JOIN clause. Parsing JOIN...\n");
        node->select.join = parse_join(p);
        node->select.join->left_table = node->select.table;
    }

    // Case select statement has a WHERE clause
    if(p->current.type == TOK_WHERE) {
        // printf("Select has WHERE clause. Parsing WHERE...\n");
        node->select.where = parse_where(p);
    }
    
    return node;
}

static ASTNode *parse_delete(Parser *p)
{
    expect(p, TOK_DELETE, "Expected DELETE");
    expect(p, TOK_FROM, "Expected FROM");

    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = AST_DELETE;

    if(p->current.type != TOK_IDENTIFIER)
        error("Expected table name");
    // printf("Table name: %s\n", p->current.lexme);
    node->delete.table_name = strdup(p->current.lexme);
    advance(p);

    // delete statement has a WHERE clause
    if(p->current.type == TOK_WHERE) {
        // printf("Delete has WHERE clause. Parsing WHERE...\n");
        node->delete.where = parse_where(p);
    }
    
    return node;
}


static ASTNode *parse_update(Parser *p)
{
    expect(p, TOK_UPDATE, "Expected UPDATE");

    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = AST_UPDATE;

    if(p->current.type != TOK_IDENTIFIER)
        error("Expected table name");
    // printf("Table name: %s\n", p->current.lexme);
    node->update.table_name = strdup(p->current.lexme);
    advance(p);
    
    expect(p, TOK_SET, "Expected SET");

    UpdateAssign assigns[16];
    int assign_count = 0;
    while(1)
    {
        if(p->current.type != TOK_IDENTIFIER)
            error("Expected column name");
        // printf("Column name: %s\n", p->current.lexme);
        Token col = p->current;
        assigns[assign_count].column = strdup(col.lexme);
        advance(p);

        expect(p, TOK_EQUAL, "Expected EQUAL");

        Token val;
        if(p->current.type == TOK_STRING || p->current.type == TOK_NUMBER)
        {
            val = p->current;
            // printf("Column value: %s\n", val.lexme);
            assigns[assign_count].value = strdup(val.lexme);
            advance(p);
        }
        else
        {
            error("Expected literal in SET clause");
            return NULL;
        }

        assign_count++; // Increment assigns before checking next token
        
        if(p->current.type != TOK_COMMA)
            break;
        advance(p);

        
    }

    // update statement has a WHERE clause (ideally on primary key)
    if(p->current.type == TOK_WHERE) {
        // printf("Update has WHERE clause. Parsing WHERE...\n");
        node->update.where = parse_where(p);
    }

    node->update.assign_count = assign_count;
    node->update.assignments = malloc(sizeof(UpdateAssign) * assign_count);
    memcpy(node->update.assignments, assigns, sizeof(UpdateAssign) * assign_count);

    return node;
}


ASTNode *parse_statement(Parser *p)
{
    if(p->current.type == TOK_CREATE)
    {
        advance(p);
        return parse_create(p);
    }
    if(p->current.type == TOK_INSERT)
    {
        advance(p);
        return parse_insert(p);
    }
    if(p->current.type == TOK_SELECT)
    {
        advance(p);
        return parse_select(p);
    }
    if(p->current.type == TOK_UPDATE)
    {
        // advance(p);
        return parse_update(p);
    }    
    if(p->current.type == TOK_DELETE)
    {
        // advance(p);
        return parse_delete(p);
    }

    error("Unsupported statement");
    return NULL;
}


void free_statement(Statement *stmt)
{
    free(stmt);
}
