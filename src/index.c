#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "index.h"


static uint32_t hash_string(const char *s)
{
    uint32_t h = 5381;

    while(*s)
        h = ((h << 5) + h) ^ (uint8_t)*s++;
    return h;
}


int index_create_pk(const char *table)
{
    char path[128];
    snprintf(path, sizeof(path), "../data/%s.pk.idx", table);

    FILE *f = fopen(path, "wb");
    if(!f) return -1;

    uint32_t buckets = PK_BUCKETS;
    fwrite(&buckets, sizeof(buckets), 1, f);

    for(uint32_t i = 0; i < buckets; i++)
    {
        uint32_t zero = 0;
        fwrite(&zero, sizeof(zero), 1, f);
    }

    fclose(f);

    return 0;
}


int index_insert_pk(const char *table, const char *key, uint64_t offset)
{
    char path[128];
    snprintf(path, sizeof(path), "../data/%s.pk.idx", table);

    FILE *f = fopen(path, "rb+");
    if(!f) return -1;

    uint32_t bucket_count;
    fread(&bucket_count, sizeof(bucket_count), 1, f);

    uint32_t h = hash_string(key) % bucket_count;

    /* Seek to bucket header */
    long bucket_pos = sizeof(uint32_t) + h * sizeof(uint32_t);
    fseek(f, bucket_pos, SEEK_SET);

    uint32_t count;
    fread(&count, sizeof(count), 1, f);

    /* Seek to end of bucket */
    long entries_pos = bucket_pos + sizeof(uint32_t);
    fseek(f, entries_pos, SEEK_SET);

    for(uint32_t i = 0; i < count; i++)
    {
        uint32_t len;
        fread(&len, sizeof(len), 1, f);
        fseek(f, len + sizeof(uint64_t), SEEK_CUR);
    }

    /* Append entry */
    uint32_t key_len = strlen(key);
    fwrite(&key_len, sizeof(key_len), 1, f);
    fwrite(&key, 1, key_len, f);
    fwrite(&offset, sizeof(offset), 1, f);

    /* Update count */
    count++;
    fseek(f, bucket_pos, SEEK_SET);
    fwrite(&count, sizeof(count), 1, f);

    fclose(f);

    return 0;
}


int index_find_pk(const char *table, const char *key, uint64_t *out_offset)
{
    char path[128];
    snprintf(path, sizeof(path), "../data/%s.pk.idx", table);

    FILE *f = fopen(path, "rb");
    if(!f) return -1;

    uint32_t bucket_count;
    fread(&bucket_count, sizeof(bucket_count), 1, f);

    uint32_t h = hash_string(key) % bucket_count;

    /* Seek to bucket header */
    long bucket_pos = sizeof(uint32_t) + h * sizeof(uint32_t);
    fseek(f, bucket_pos, SEEK_SET);

    uint32_t count;
    fread(&count, sizeof(count), 1, f);

    for(uint32_t i = 0; i < count; i++)
    {
        uint32_t len;
        fread(&len, sizeof(len), 1, f);

        char buf[256];
        fread(buf, 1, len, f);
        buf[len] = 0;

        uint64_t off;
        fread(&off, sizeof(off), 1, f);

        if(strcmp(buf, key) == 0)
        {
            *out_offset = off;
            fclose(f);
            return 0;
        }
    }

    fclose(f);

    return 1;
}

