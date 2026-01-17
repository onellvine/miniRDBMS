#ifndef __INDEX_H__
#define __INDEX_H__

#include <stdint.h>


#define PK_BUCKETS 1024

static uint32_t hash_string(const char *s);

int index_create_pk(const char *table);
int index_insert_pk(const char *table, const char *key, uint64_t offset);
int index_find_pk(const char *table, const char *key, uint64_t *out_offset);

#endif /* __INDEX_H__ */