#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>

#include "../include/util.h"

static int jz_size_add_checked(size_t a, size_t b, size_t *out)
{
    if (!out) return -1;
    if (a > SIZE_MAX - b) return -1;
    *out = a + b;
    return 0;
}

char *jz_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    size_t alloc_size = 0;
    if (jz_size_add_checked(len, 1, &alloc_size) != 0) return NULL;
    char *copy = (char *)malloc(alloc_size);
    if (!copy) return NULL;
    memcpy(copy, s, len + 1);
    return copy;
}

char *jz_read_entire_file(const char *filename, size_t *out_size) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    long len = ftell(f);
    if (len < 0) {
        fclose(f);
        return NULL;
    }
    rewind(f);

    size_t alloc_size = 0;
    if (jz_size_add_checked((size_t)len, 1, &alloc_size) != 0) {
        fclose(f);
        return NULL;
    }

    char *buf = (char *)malloc(alloc_size);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    size_t read = fread(buf, 1, (size_t)len, f);
    fclose(f);
    if (read != (size_t)len) {
        free(buf);
        return NULL;
    }

    buf[len] = '\0';
    if (out_size) *out_size = (size_t)len;
    return buf;
}

int jz_buf_reserve(JZBuffer *buf, size_t new_cap)
{
    if (!buf) return -1;
    if (new_cap <= buf->cap) return 0;
    size_t cap = buf->cap ? buf->cap : 16;
    while (cap < new_cap) {
        if (cap > SIZE_MAX / 2) {
            cap = new_cap;
            break;
        }
        cap *= 2;
    }
    unsigned char *data = (unsigned char *)realloc(buf->data, cap);
    if (!data) return -1;
    buf->data = data;
    buf->cap = cap;
    return 0;
}

int jz_buf_append(JZBuffer *buf, const void *data, size_t len)
{
    if (!buf || (!data && len != 0)) return -1;
    if (len == 0) return 0;
    size_t new_len = 0;
    if (jz_size_add_checked(buf->len, len, &new_len) != 0) return -1;
    if (jz_buf_reserve(buf, new_len) != 0) return -1;
    memcpy(buf->data + buf->len, data, len);
    buf->len = new_len;
    return 0;
}

void jz_buf_free(JZBuffer *buf)
{
    if (!buf) return;
    free(buf->data);
    buf->data = NULL;
    buf->len = 0;
    buf->cap = 0;
}
