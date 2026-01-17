#ifndef __STORAGE_H__
#define __STORAGE_H__

#include <stdint.h>

#include "ast.h"


int storage_create_table(CreateTableStmt *stmt);
int storage_insert(InsertStmt *stmt);
int storage_scan(
    const char *table,
    void (*row_cb)(char **values, int count, void *ctx),
    void *ctx
);
int storage_delete_by_pk(const char *table, Expr *pk_value);
int storage_update_by_pk(const char *table, Expr *pk_value, UpdateAssign *assigns, int assign_count);

#endif /* __STORAGE_H__ */