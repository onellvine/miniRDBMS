#ifndef __EXECUTOR_H__
#define __EXECUTOR_H__

#include "ast.h"


typedef struct _PKCheckCtx {
    int pk_index;
    const char *pk_value;
    int duplicate_found;
} PKCheckCtx;

typedef struct _UniqueCheckCtx {
    int column_index;
    const char *value;
    int violation;
} UniqueCheckCtx;

typedef struct _JoinCtx {
    const char *right_table;
    int left_join_idx;
    int right_join_idx;
    int left_count;
    char **values;
} JoinCtx;

typedef struct _SelectCtx {
    SelectStmt *stmt;
    TableSchema *schema;
    int *proj_cols;
    int proj_count;
} SelectCtx;

static int where_matches(SelectStmt *s, TableSchema *schema, char **values);

static void pk_check_cb(char **values, int count, void *ctx);
static void unique_check_cb(char **values, int count, void *ctx);
static void join_right_cb(char **rvals, int rcount, void *ctx);
static void join_left_cb(char **lvals, int lcount, void *ctx);
static void select_row_cb(char **values, int count, void *ctx);


void execute(ASTNode *node);

#endif /* __EXECUTOR_H__ */