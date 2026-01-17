#ifndef __AST_H__
#define __AST_H__

#include <stdlib.h>

#include "token.h"

typedef enum {
    AST_SELECT,
    AST_INSERT,
    AST_CREATE,
    AST_DELETE,
    AST_UPDATE
} ASTNodeType;

typedef enum {
    EXPR_LITERAL,
    EXPR_COLUMN
} ExprType;

typedef struct _Expr {
    ExprType type;
    char *value;
} Expr;

/* --------------- CREATE TABLE ---------------- */
typedef enum {
    COL_INT = 1,
    COL_TEXT,
    COL_BOOL
} ColumnType;

typedef struct _ColumnDef {
    char *name;
    ColumnType type;
    int primary_key;
    int unique;
} ColumnDef;

typedef struct _TableSchema {
    ColumnDef *columns;
    int column_count;
} TableSchema;

typedef struct _CreateTableStmt {
    char *table_name;
    ColumnDef *columns;
    int column_count;
} CreateTableStmt;

/* --------------- INSERT ---------------- */
typedef struct _Row {
    Expr **values;
    int value_count;
} Row;

typedef struct _InsertStmt {
    char *table_name;
    Row row;
} InsertStmt;

/* WHERE column = literal */
typedef struct _WhereClause {
    char *coulmn;
    Expr *value;
} WhereClause;

typedef struct _JoinClause {
    char *left_table;
    char *right_table;
    char *left_column;
    char *right_column;
} JoinClause;

/* ---------------- SELECT --------------- */
typedef struct _SelectStmt {
    int select_all; // 0 or 1 for *
    char **columns;
    int column_count;

    char *table;
    JoinClause *join;
    WhereClause *where;
} SelectStmt;

/* ---------------- DELETE --------------- */
typedef struct _DeleteStmt {
    char *table_name;
    const char *pk_value;
    WhereClause *where;
} DeleteStmt;

/* ---------------- UPDATE --------------- */
typedef struct _UpdateAssign {
    char *column;
    char *value;
} UpdateAssign;

typedef struct _UpdateStmt {
    char *table_name;
    int assign_count;
    UpdateAssign *assignments;
    WhereClause *where;
} UpdateStmt;

/* ------------------ Root ---------------- */
typedef struct _ASTNode {
    ASTNodeType type;
    union {
        CreateTableStmt create;
        InsertStmt insert;
        SelectStmt select;
        DeleteStmt delete;
        UpdateStmt update;
    };
} ASTNode;

static inline void ast_free(ASTNode *node)
{
    if(!node) return;

    if(node->type == AST_CREATE)
    {
        free(node->create.table_name);
        for(int i = 0; i < node->create.column_count; i++)
            free(node->create.columns[i].name);
        free(node->create.columns);
    }

    if(node->type == AST_INSERT)
    {
        free(node->insert.table_name);
        for(int i = 0; i < node->insert.row.value_count; i++)
        {
            free(node->insert.row.values[i]->value);
            free(node->insert.row.values[i]);
        }
        free(node->insert.row.values);
    }

    if(node->type == AST_SELECT)
    {
        for(int i = 0; i < node->select.column_count; i++)
            free(node->select.columns[i]);
        free(node->select.columns);
        free(node->select.table);

        if(node->select.where)
        {
            free(node->select.where->coulmn);
            free(node->select.where->value->value);
            free(node->select.where->value);
            free(node->select.where);
        }
    }

    free(node);
}


#endif /* __AST_H__ */
