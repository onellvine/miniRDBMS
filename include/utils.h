#ifndef __UTILS_H__
#define __UTILS_H__

#include <inttypes.h>

/* Storage schema */
typedef struct _DiskColumnMeta {
    uint8_t type;
    uint8_t primary_key;
    uint8_t unique;
} DiskColumnMeta;

char *strdup(const char *s);
char *strndup(const char *s, size_t n);

#endif /* __UTILS_H__ */