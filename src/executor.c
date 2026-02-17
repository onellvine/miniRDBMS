#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "catalog.h"
#include "executor.h"
#include "mystorage.h"
#include "utils.h"


static void pk_check_cb(char **values, int count, void *ctx)
{
    PKCheckCtx *c = ctx;

    if(strcmp(values[c->pk_index], c->pk_value) == 0)
        c->duplicate_found = 1;
}


static void unique_check_cb(char **values, int count, void *ctx)
{
    UniqueCheckCtx *c = ctx;

    if(strcmp(values[c->column_index], c->value) == 0)
        c->violation = 1;
}


static void join_right_cb(char **rvals, int rcount, void *ctx)
{
    JoinCtx *c = ctx;

    if(strcmp(c->values[c->left_join_idx], rvals[c->right_join_idx]) == 0)
    {
        // Print joined row
        for(int i = 0; i < c->left_count; i++)
            printf("%16s | ", c->values[i]);
        
        for(int i = 0; i < rcount; i++)
            printf("%16s | ", rvals[i]);
        
        printf("\n");
    }
}


static void join_left_cb(char **lvals, int lcount, void *ctx)
{
    JoinCtx *c = ctx;

    c->values = lvals;
    c->left_count = lcount;

    storage_scan(c->right_table, join_right_cb, ctx);
}


static void select_row_cb(char **values, int count, void *ctx)
{
    SelectCtx *c = ctx;

    int where_match = where_matches(c->stmt, c->schema, values);
    if(!where_match) {
        // printf("Callback NOT WHERE match: %d\n", where_match);
        return;
    }

    int padding = 4;
    for(int i = 0; i < c->proj_count; i++)
    {
        // if(c->schema->columns[i].type == COL_INT)
        //     padding = 5;
        // else if(c->schema->columns[i].type == COL_TEXT)        
        padding = get_padding(c->schema->columns[i].name);

        int idx = c->proj_cols[i];
        printf("%*s", padding, values[idx]);
        if(i + 1 < c->proj_count)
            printf(" | ");
    }
    printf("\n");
}


static int where_matches(SelectStmt *s, TableSchema *schema, char **values)
{
    if(!s->where) return 1;

    int idx = catalog_column_index(schema, s->where->coulmn);
    if(idx < 0) return 0;

    return (strcmp(values[idx], s->where->value->value) == 0);
}


static void exec_select(SelectStmt *s)
{
    TableSchema schema;
    if(catalog_load(s->table, &schema) != 0)
    {
        printf("Error: table '%s' not found\n", s->table);
        return;
    }

    int *proj_cols = NULL;
    int proj_count = 0;

    if(s->select_all)
    {
        int padding = 4;
        int column_count = schema.column_count;

        for(int i = 0; i < column_count; i++)
        {
            // if(schema.columns[i].type == COL_INT)
            //     padding = 5;
            // else if(schema.columns[i].type == COL_TEXT)
            padding = get_padding(schema.columns[i].name);

            printf("%*s", padding, schema.columns[i].name);
            if(i + 1 < column_count) printf(" | ");
        }
        printf("\n");

        proj_count = column_count; // schema.column_count;
        proj_cols = malloc(sizeof(int) * proj_count);
        for(int i = 0; i < proj_count; i++)
            proj_cols[i] = i;
    }
    else
    {
        proj_count = s->column_count;
        proj_cols = malloc(sizeof(int) * proj_count);

        int padding = 4;
        int column_count = proj_count;

        for(int i = 0; i < column_count; i++)
        {

            int n = 0;
            for( ; n < schema.column_count ; )
            {
                if(strcmp(s->columns[i], schema.columns[n].name) == 0) {
                    // if(schema.columns[n].type == COL_INT)
                    //     padding = 5;
                    // else if(schema.columns[n].type == COL_TEXT)
                    padding = get_padding(schema.columns[n].name);
                }
                n++;
            }

            printf("%*s", padding, s->columns[i]);
            if(i + 1 < column_count) printf(" | ");
        }
        printf("\n");

        for(int i = 0; i < proj_count; i++)
        {
            int idx = catalog_column_index(&schema, s->columns[i]);
            // printf("Column index: %d\n", idx);
            if(idx < 0)
            {
                printf("Error: unknown column [specified]: '%s'\n", s->columns[i]);
                goto cleanup;
            }
            proj_cols[i] = idx;
        }
    }

    if(s->where)
    {
        if(catalog_column_index(&schema, s->where->coulmn) < 0)
        {
            printf("Error: unknown column [in WHERE]: '%s'\n", s->where->coulmn);
            goto cleanup;            
        }
    }

    SelectCtx ctx = {
        .stmt = s,
        .schema = &schema,
        .proj_cols = proj_cols,
        .proj_count = proj_count
    };

    storage_scan(s->table, select_row_cb, &ctx);

cleanup:
    free(proj_cols);
    catalog_free(&schema);
}


static void execute_select_join(SelectStmt *s)
{
    TableSchema left, right;

    if(catalog_load(s->table, &left))
    {
        printf("Error: from table not found\n");
        return;
    }
    if(catalog_load(s->join->right_table, &right))
    {
        printf("Error: right table not found\n");
        return;
    }

    int left_idx = -1, right_idx = -1;

    for(int i = 0; i < left.column_count; i++)
        if(strcmp(left.columns[i].name, s->join->left_column) == 0)
            left_idx = i;
    for(int i = 0; i < right.column_count; i++)
        if(strcmp(right.columns[i].name, s->join->right_column) == 0)
            right_idx = i;

    if(left_idx < 0 || right_idx < 0)
    {
        printf("Error: join column not found\n");
        return;
    }

    JoinCtx ctx = {
        .right_table = s->join->right_table,
        .left_join_idx = left_idx,
        .right_join_idx = right_idx
    };

    storage_scan(s->table, join_left_cb, &ctx);

    catalog_free(&left);
    catalog_free(&right);
}


void execute(ASTNode *node)
{
    TableSchema schema;
    int pk;

    switch(node->type)
    {
        case AST_CREATE:
            if(storage_create_table(&node->create) == 0)
                printf("Table created\n");
            else
                printf("Create table failed\n");
            break;

        case AST_INSERT:
            
            if(catalog_load(node->insert.table_name, &schema))
            {
                printf("Error: table not found\n");
                break;
            }
            if(schema.column_count != node->insert.row.value_count)
            {
                printf("Error: column count mismatch\n");
                catalog_free(&schema);
                break;
            }
            int pk_index = catalog_primary_key_index(&schema);
            if(pk_index == -2)
            {
                printf("Error: multiple primary keys defined\n");
                catalog_free(&schema);
                break;
            }
            if(pk_index >= 0)
            {
                const char *pk_value = node->insert.row.values[pk_index]->value;
                PKCheckCtx ctx = {
                    .pk_index = pk_index,
                    .pk_value = pk_value,
                    .duplicate_found = 0
                };

                storage_scan(node->insert.table_name, pk_check_cb, &ctx);

                if(ctx.duplicate_found)
                {
                    printf("Error: duplicate primary key value '%s'\n", pk_value);
                    catalog_free(&schema);
                    break;
                }
            }
            /* UNIQUE column enforcement */
            int unique_cols[16];
            int unique_count = catalog_unique_indices(&schema, unique_cols, 16);

            for(int i = 0; i < unique_count; i++)
            {
                int col = unique_cols[i];
                const char *val = node->insert.row.values[col]->value;

                UniqueCheckCtx uctx = {
                    .column_index = col,
                    .value = val,
                    .violation = 0
                };

                storage_scan(node->insert.table_name, unique_check_cb, &uctx);

                if(uctx.violation)
                {
                    printf("Error: duplicate value '%s' for UNIQUE column '%s'\n", 
                        val, schema.columns[col].name);
                    catalog_free(&schema);
                    break;
                }
            }

            if(schema.column_count != node->insert.row.value_count)
            {
                catalog_free(&schema);
                break;
            }

            catalog_free(&schema);
            if(storage_insert(&node->insert) == 0)
                printf("Row inserted\n");
            else
                printf("Insert row failed\n");
            break;
        case AST_SELECT:
            if(node->select.join)
                execute_select_join(&node->select);
            else
                exec_select(&node->select);
            break;
        case AST_UPDATE:
        {
            if(catalog_load(node->update.table_name, &schema))
            {
                printf("Error: table not found\n");
                break;
            }
            int pk = catalog_primary_key_index(&schema);
            if(pk < 0 || strcmp(schema.columns[pk].name, node->update.where->coulmn) != 0)
            {
                printf("Error: UPDATE requires WHERE on PRIMARY KEY\n");
                break;
            }
            /* Prevent PK modification */ 
            for(int i = 0; i < node->update.assign_count; i++)
            {
                if(strcmp(node->update.assignments[i].column, schema.columns[pk].name) == 0)
                {
                    printf("Error: cannot update PRIMARY KEY\n");
                    catalog_free(&schema);
                    break;
                }
            }
            int rc = storage_update_by_pk(
                node->update.table_name,
                node->update.where->value,
                node->update.assignments,
                node->update.assign_count
            );
            if(rc == 0)
                printf("Row updated\n");
            else
                printf("Row not found\n");
            break;
        }
        case AST_DELETE:
            if(catalog_load(node->delete.table_name, &schema))
            {
                printf("Error: table not found\n");
                break;
            }
            pk = catalog_primary_key_index(&schema);
            if(pk < 0 || strcmp(schema.columns[pk].name, node->delete.where->coulmn) != 0)
            {
                printf("Error: DELETE requires WHERE on PRIMARY KEY\n");
                break;
            }
            if(storage_delete_by_pk(node->delete.table_name, node->delete.where->value) == 0)
            {
                printf("Row deleted\n");
            }
            else
            {
                printf("Row not found");
            }
            break;        
        default:
            printf("Execution not implemented\n");
            break;
    }
}