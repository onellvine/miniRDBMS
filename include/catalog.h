#ifndef __CATALOG_H__
#define __CATALOG_H__

#include "ast.h"


int catalog_load(const char *table_name, TableSchema *out);
int catalog_column_index(TableSchema *schema, const char *name);
int catalog_primary_key_index(TableSchema *schema);
int catalog_unique_indices(TableSchema *schema, int *out, int max);
void catalog_free(TableSchema *schema);

#endif /* __CATALOG_H__ */