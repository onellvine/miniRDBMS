#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "index.h"
#include "mystorage.h"
#include "utils.h"


static int skip_schema(FILE *f)
{
    uint32_t column_count;
    if(fread(&column_count, sizeof(column_count), 1, f) != 1)
        return -1;
    
    for(uint32_t i = 0; i < column_count; i++)
    {
        uint32_t name_len;
        fread(&name_len, sizeof(name_len), 1, f);
        fseek(f, name_len, SEEK_CUR); // skip name
        fseek(f, 3, SEEK_CUR); // skip type + primary_key + unique
    }

    return 0;
}


static int copy_schema(FILE *src, FILE *dst)
{
    uint32_t column_count;
    if(fread(&column_count, sizeof(column_count), 1, src) != 1)
        return -1;

    fwrite(&column_count, sizeof(column_count), 1, dst);

    for(uint32_t i = 0; i < column_count; i++)
    {
        uint32_t len;
        fread(&len, sizeof(len), 1, src);
        fwrite(&len, sizeof(len), 1, dst);

        char *name = malloc(len);
        fread(name, 1, len, src);
        fwrite(name, 1, len, dst);
        free(name);

        uint8_t meta[3];
        fread(meta, 1, 3, src);
        fwrite(meta, 1, 3, dst);
    }

    return 0;
}


static int rebuild_pk_index(const char *table)
{
    char tbl[128], idx[128];
    snprintf(tbl, sizeof(tbl), "../data/%s.tbl", table);
    snprintf(idx, sizeof(idx), "../data/%s.pk.idx", table);

    remove(idx);
    index_create_pk(table);

    FILE *f = fopen(tbl, "rb");
    if(!f) return -1;

    /* read schema */
    uint32_t cols;
    fread(&cols, sizeof(cols), 1, f);

    int pk_index = -1;
    for(uint32_t i = 0; i < cols; i++)
    {
        uint32_t len;
        fread(&len, sizeof(len), 1, f);
        fseek(f, len, SEEK_CUR);

        uint8_t type, pk, unique;
        fread(&type, 1, 1, f);
        fread(&pk, 1, 1, f);
        fread(&unique, 1, 1, f);

        if(pk) pk_index = i;
    }

    if(pk_index < 0)
    {
        fclose(f);
        return 0;
    }

    while(1)
    {
        uint64_t row_offset = ftell(f);

        uint32_t count;
        if(fread(&count, sizeof(count), 1, f) != 1)
            break;
        
        char **values = malloc(sizeof(char *) * count);
        for(uint32_t i = 0; i < count; i++)
        {
            uint32_t len;
            fread(&len , sizeof(len), 1, f);
            values[i] = malloc(len + 1);
            fread(values[i], 1, len, f);
            values[i][len] = '\0';
        }

        index_insert_pk(table, values[pk_index], row_offset);

        for(uint32_t i = 0; i < count; i++)
            free(values[i]);
        free(values);
    }

    fclose(f);

    return 0;
}


int storage_create_table(CreateTableStmt *stmt)
{
    char file[128];
    // table_filename(file, stmt->table_name);
    snprintf(file, sizeof(file), "../data/%s.tbl", stmt->table_name);

    FILE *f = fopen(file, "wb");
    if(!f) {
        perror("Create table [open]");
        return -1;
    }
    // printf("Create table stmt col count: %d\n", stmt->column_count);
    uint32_t col_count = stmt->column_count;
    fwrite(&col_count, sizeof(col_count), 1, f);
    // fwrite(stmt->columns, sizeof(ColumnDef), stmt->column_count, f);
    for(int i = 0; i < stmt->column_count; i++)
    {
        ColumnDef *c = &stmt->columns[i];
        
        uint32_t name_len = strlen(c->name);
        fwrite(&name_len, sizeof(name_len), 1, f);
        fwrite(c->name, 1, name_len, f);

        DiskColumnMeta meta = {
            .type = (uint8_t)c->type,
            .primary_key = (uint8_t)c->primary_key,
            .unique = (uint8_t)c->unique
        };

        // fwrite(&c->type, sizeof(int), 1, f);
        // fwrite(&c->primary_key, sizeof(int), 1, f);
        // fwrite(&c->unique, sizeof(int), 1, f);
        fwrite(&meta, sizeof(meta), 1, f);
    }

    fclose(f);

    return 0;
}


int storage_insert(InsertStmt *insert)
{
    char file[128];
    snprintf(file, sizeof(file), "../data/%s.tbl", insert->table_name);

    FILE *f = fopen(file, "ab");
    if(!f) return -1;

    /* Read schema to find PK index */
    uint32_t column_count;
    fread(&column_count, sizeof(column_count), 1, f);

    int pk_index = -1;
    for(uint32_t i = 0; i < column_count; i++)
    {
        uint32_t name_len;
        fread(&name_len, sizeof(name_len), 1, f);
        fseek(f, name_len, SEEK_CUR);

        uint8_t type, pk, unique;
        fread(&type, 1, 1, f);
        fread(&pk, 1, 1, f);
        fread(&unique, 1, 1, f);

        if(pk)
            pk_index = i;
    }

    /* Seek to end of file (append row) */
    fseek(f, 0, SEEK_END);
    uint64_t row_offset = ftell(f);

    /* Write row */
    uint32_t n = insert->row.value_count;
    fwrite(&n, sizeof(n), 1, f);

    for(uint32_t i = 0; i < n ; i++)
    {
        uint32_t len = strlen(insert->row.values[i]->value);
        fwrite(&len, sizeof(len), 1, f);
        fwrite(insert->row.values[i]->value, 1, len, f);
    }

    /* Insert into PK index */
    if(pk_index >= 0)
    {
        const char *pk_value = insert->row.values[pk_index]->value;
        index_insert_pk(insert->table_name, pk_value, row_offset);
    }

    fclose(f);

    return 0;
}


int storage_scan(
    const char *table,
    void (*row_cb)(char **values, int count, void *ctx),
    void *ctx
)
{
    char file[128];
    snprintf(file, sizeof(file), "../data/%s.tbl", table);

    FILE *f = fopen(file, "rb");
    if(!f){
        perror("Storage Scan [open]: ");
        return -1;
    }

    if(skip_schema(f) != 0)
    {
        fclose(f);
        return -1;
    }

    while(!feof(f))
    {
        uint32_t count;
        if(fread(&count, sizeof(count), 1, f) != 1) {
            // printf("SS read 1 failed\n");
            break;
        }

        char **values = malloc(sizeof(char *) * count);
        for(uint32_t i = 0; i < count; i++)
        {
            uint32_t len;
            fread(&len, sizeof(len), 1, f);

            values[i] = malloc(len + 1);
            fread(values[i], 1, len, f);
            values[i][len] = 0;
        }

        row_cb(values, count, ctx);

        for(uint32_t i = 0; i < count; i++)
            free(values[i]);
        free(values);
    }

    fclose(f);

    return 0;
}


int storage_delete_by_pk(const char *table, Expr *pk_value)
{
    char src_path[128], tmp_path[128];
    snprintf(src_path, sizeof(src_path), "../data/%s.tbl", table);
    snprintf(tmp_path, sizeof(tmp_path), "../data/%s.tbl.tmp", table);

    FILE *src = fopen(src_path, "rb");
    if(!src) return -1;

    FILE *dst = fopen(tmp_path, "wb");
    if(!dst){
        fclose(src);
        return -1;
    }

    /* Copy schema */
    if(copy_schema(src, dst) != 0)
    {
        fclose(src);
        fclose(dst);
        return -1;
    }

    /* Determine PK index */
    fseek(src, 0, SEEK_SET);
    uint32_t cols;
    fread(&cols, sizeof(cols), 1, src);

    int pk_index = -1;
    for(uint32_t i = 0; i < cols; i++)
    {
        uint32_t len;
        fread(&len, sizeof(len), 1, src);
        fseek(src, len, SEEK_CUR);


        uint8_t type, pk, unique;
        fread(&type, 1, 1, src);
        fread(&pk, 1, 1, src);
        fread(&unique, 1, 1, src);

        if(pk) pk_index = i;
    }

    int deleted = 0;

    while(1)
    {
        uint32_t count;
        if(fread(&count, sizeof(count), 1, src) != 1)
            break;

        char **values = malloc(sizeof(char *) * count);
        for(uint32_t i = 0; i < count; i++)
        {
            uint32_t len;
            fread(&len, sizeof(len), 1, src);
            values[i] = malloc(len + 1);
            fread(values[i], 1, len, src);
            values[i][len] = 0;
        }

        if(pk_index >= 0 && strcmp(values[pk_index], pk_value->value) == 0)
        {
            deleted = 1;
        }
        else
        {
            fwrite(&count, sizeof(count), 1, dst);
            for(uint32_t i = 0; i < count; i++)
            {
                uint32_t len = strlen(values[i]);
                fwrite(&len, sizeof(len), 1, dst);
                fwrite(values[i], 1, len, dst);
            }
        }

        for(uint32_t i = 0; i < count; i++)
            free(values[i]);
        free(values);
    }

    fclose(src);
    fclose(dst);

    if(!deleted)
    {
        remove(tmp_path);
        return 1; // nothing deleted
    }

    /* Replace table */
    remove(src_path);
    rename(tmp_path, src_path);

    /* Rebuild PK index */
    rebuild_pk_index(table);

    return 0;
}


int storage_update_by_pk(const char *table, Expr *pk_value, UpdateAssign *assigns, int assign_count)
{
    char src_path[128], tmp_path[128];
    snprintf(src_path, sizeof(src_path), "../data/%s.tbl", table);
    snprintf(tmp_path, sizeof(tmp_path), "../data/%s.tbl.tmp", table);

    FILE *src = fopen(src_path, "rb");
    if(!src) return -1;

    FILE *dst = fopen(tmp_path, "wb");
    if(!dst){
        fclose(src);
        return -1;
    }

    /* Copy schema */
    if(copy_schema(src, dst) != 0)
    {
        fclose(src);
        fclose(dst);
        return -1;
    }

    /* Determine PK index */
    fseek(src, 0, SEEK_SET);
    uint32_t cols;
    fread(&cols, sizeof(cols), 1, src);

    int pk_index = -1;
    char *col_names[64];

    for(uint32_t i = 0; i < cols; i++)
    {
        uint32_t len;
        fread(&len, sizeof(len), 1, src);

        col_names[i] = malloc(len + 1);
        fread(col_names[i], 1, len, src);
        col_names[i][len] = '\0';
        printf("col name[%i]: %s\n", i, col_names[i]);

        uint8_t meta[3];
        fread(meta, 1, 3, src);

        if(meta[1]) pk_index = i;
    }

    int updated = 0;

    while(1)
    {
        uint32_t count;
        if(fread(&count, sizeof(count), 1, src) != 1)
            break;

        char **values = malloc(sizeof(char *) * count);
        for(uint32_t i = 0; i < count; i++)
        {
            uint32_t len;
            fread(&len, sizeof(len), 1, src);
            values[i] = malloc(len + 1);
            fread(values[i], 1, len, src);
            values[i][len] = '\0';
            printf("values[%i]: %s\n", i, values[i]);
        }
        
        if(strcmp(values[pk_index], pk_value->value) == 0)
        {
            for(int a = 0; a < assign_count; a++)
            {
                for(uint32_t c = 0; c < count; c++)
                {
                    if(strcmp(assigns[a].column, col_names[c]) == 0)
                    {
                        printf("cvalues[%i]: %s\n", c, values[c]);
                        free(values[c]);
                        values[c] = strdup(assigns[a].value);
                        printf("cvalues[%i]: %s\n", c, values[c]);
                    }
                }
            }
            updated = 1;
        }

        fwrite(&count, sizeof(count), 1, dst);
        for(uint32_t i = 0; i < count; i++)
        {
            uint32_t len = strlen(values[i]);
            fwrite(&len, sizeof(len), 1, dst);
            fwrite(values[i], 1, len, dst);
        }

        for(uint32_t i = 0; i < count; i++)
            free(values[i]);
        free(values);
    }

    for(uint32_t i = 0; i < cols; i++)
        free(col_names[i]);

    fclose(src);
    fclose(dst);

    if(!updated)
    {
        remove(tmp_path);
        return 1;
    }

    remove(src_path);
    rename(tmp_path, src_path);

    rebuild_pk_index(table);

    return 0;
}
