#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "catalog.h"
#include "utils.h"


int catalog_load(const char *table_name, TableSchema *out)
{
    char file[128];
    snprintf(file, sizeof(file), "../data/%s.tbl", table_name);
    // printf("Loading from %s\n", file);

    FILE *f = fopen(file, "r");
    if(!f) return -1;

    uint32_t col_count;
    fread(&col_count, sizeof(col_count), 1, f);

    out->column_count = col_count;
    // printf("Catalog load %d columns\n", out->column_count);

    out->columns = calloc(col_count, sizeof(ColumnDef));
    // fread(out->columns, sizeof(ColumnDef), out->column_count, f);

    for(uint32_t i = 0; i < col_count; i++)
    {
        uint32_t name_len;
        fread(&name_len, sizeof(name_len), 1, f);

        out->columns[i].name = malloc(name_len + 1);
        fread(out->columns[i].name, 1, name_len, f);
        out->columns[i].name[name_len] = 0;

        DiskColumnMeta meta;
        fread(&meta, sizeof(meta), 1, f);

        out->columns[i].type = meta.type;
        out->columns[i].primary_key = meta.primary_key;
        out->columns[i].unique = meta.unique;
    }

    fclose(f);

    return 0;
}


void catalog_free(TableSchema *schema)
{
    // free(schema->columns);
    for(int i = 0; i < schema->column_count; i++)
        free(schema->columns[i].name);
    free(schema->columns);
}


int catalog_column_index(TableSchema *schema, const char *name)
{
    // printf("Schema column count: %d\n", schema->column_count);
    for(int i = 0; i < schema->column_count; i++)
    {
        // printf("Schema column %i: %s <==> name: %s\n", i, schema->columns[i], name);
        if(strcmp(schema->columns[i].name, name) == 0)
            return i;
    }

    return -1;
}


int catalog_primary_key_index(TableSchema *schema)
{
    int pk = -1;
    for(int i = 0; i < schema->column_count; i++)
    {
        if(schema->columns[i].primary_key)
        {
            if(pk != -1)  // Multiple PKs - schema corruption imminent
                return -2;
            pk = i;
        }
    }

    return pk;
}


int catalog_unique_indices(TableSchema *schema, int *out, int max)
{
    int n = 0;
    for(int i = 0; i < schema->column_count && n < max; i++)
    {
        if(schema->columns[i].unique && !schema->columns[i].primary_key)
            out[n++] = i;
    }
    return n;
}


