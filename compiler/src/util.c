/**
 * @file util.c
 * @brief General-purpose helpers for limits, file I/O, and output staging.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <errno.h>

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#endif

#include "../include/util.h"

#ifndef _WIN32
/**
 * @brief Open a file descriptor using exclusive create semantics.
 * @param path Path to create.
 * @return Writable file descriptor, or `-1` on failure.
 */
static int jz_open_exclusive_fd(const char *path);
/**
 * @brief Build a temporary output path beside a target path.
 * @param target Final output path.
 * @param attempt Retry suffix used to keep paths unique.
 * @param tmp_path Destination buffer for the generated path.
 * @param tmp_path_size Size of @p tmp_path in bytes.
 * @return `0` on success, or `-1` on failure.
 */
static int jz_build_temp_path(const char *target,
                              unsigned attempt,
                              char *tmp_path,
                              size_t tmp_path_size);
#endif

size_t jz_input_limit_value(JZInputLimitKind kind)
{
    switch (kind) {
    case JZ_LIMIT_SOURCE_FILE_BYTES:
        return JZ_MAX_SOURCE_FILE_BYTES;
    case JZ_LIMIT_SOURCE_TOKENS:
        return JZ_MAX_SOURCE_TOKENS;
    case JZ_LIMIT_IMPORT_DEPTH:
        return JZ_MAX_IMPORT_DEPTH;
    case JZ_LIMIT_IMPORT_RETAINED_SOURCE_BYTES:
        return JZ_MAX_IMPORT_RETAINED_SOURCE_BYTES;
    case JZ_LIMIT_IMPORT_RETAINED_TOKEN_BYTES:
        return JZ_MAX_IMPORT_RETAINED_TOKEN_BYTES;
    case JZ_LIMIT_CHIP_JSON_BYTES:
        return JZ_MAX_LOCAL_CHIP_JSON_BYTES;
    case JZ_LIMIT_CHIP_JSON_TOKENS:
        return JZ_MAX_LOCAL_CHIP_JSON_TOKENS;
    case JZ_LIMIT_CHIP_JSON_NESTING_DEPTH:
        return JZ_MAX_CHIP_JSON_NESTING_DEPTH;
    case JZ_LIMIT_MEM_INIT_FILE_BYTES:
        return JZ_MAX_MEM_INIT_FILE_BYTES;
    case JZ_LIMIT_MEM_INIT_MIF_DEPTH:
        return JZ_MAX_MEM_INIT_MIF_DEPTH;
    case JZ_LIMIT_PARSER_EXPR_DEPTH:
        return JZ_MAX_PARSER_EXPR_DEPTH;
    case JZ_LIMIT_PARSER_STATEMENT_DEPTH:
        return JZ_MAX_PARSER_STATEMENT_DEPTH;
    case JZ_LIMIT_CONST_EVAL_DEPTH:
        return JZ_MAX_CONST_EVAL_DEPTH;
    case JZ_LIMIT_IR_EXPR_DEPTH:
        return JZ_MAX_IR_EXPR_DEPTH;
    case JZ_LIMIT_IR_STATEMENT_DEPTH:
        return JZ_MAX_IR_STMT_DEPTH;
    case JZ_LIMIT_AST_DEPTH:
        return JZ_MAX_AST_DEPTH;
    case JZ_LIMIT_TEMPLATE_EXPAND_DEPTH:
        return JZ_MAX_TEMPLATE_EXPAND_DEPTH;
    case JZ_LIMIT_TRISTATE_DEPTH:
        return JZ_MAX_TRISTATE_DEPTH;
    case JZ_LIMIT_SEM_RECURSION_DEPTH:
        return JZ_MAX_SEM_RECURSION_DEPTH;
    case JZ_LIMIT_REPORT_RECURSION_DEPTH:
        return JZ_MAX_REPORT_RECURSION_DEPTH;
    case JZ_LIMIT_SIM_MEMORY_DEPTH:
        return JZ_MAX_SIM_MEMORY_DEPTH;
    case JZ_LIMIT_SIM_MEMORY_OBJECT_BYTES:
        return JZ_MAX_SIM_MEMORY_OBJECT_BYTES;
    case JZ_LIMIT_EMITTED_TRACE_BYTES:
        return JZ_MAX_EMITTED_TRACE_BYTES;
    default:
        return 0;
    }
}

int jz_depth_enter_checked(unsigned *depth, JZInputLimitKind kind)
{
    size_t limit = 0;

    if (!depth) return -1;
    limit = jz_input_limit_value(kind);
    if (limit == 0 || (size_t)(*depth) >= limit) return -1;
    ++(*depth);
    return 0;
}

void jz_depth_leave(unsigned *depth)
{
    if (depth && *depth > 0) {
        --(*depth);
    }
}

int jz_size_add_checked(size_t a, size_t b, size_t *out)
{
    if (!out) return -1;
    if (a > SIZE_MAX - b) return -1;
    *out = a + b;
    return 0;
}

int jz_size_mul_checked(size_t a, size_t b, size_t *out)
{
    if (!out) return -1;
    if (a != 0 && b > SIZE_MAX / a) return -1;
    *out = a * b;
    return 0;
}

int jz_size_mul_add_checked(size_t a, size_t b, size_t c, size_t *out)
{
    size_t product = 0;

    if (!out) return -1;
    if (jz_size_mul_checked(a, b, &product) != 0) return -1;
    return jz_size_add_checked(product, c, out);
}

int jz_size_align_up_checked(size_t size, size_t alignment, size_t *out)
{
    size_t rounded = 0;

    if (!out || alignment == 0) return -1;
    if ((alignment & (alignment - 1u)) != 0u) return -1;
    if (jz_size_add_checked(size, alignment - 1u, &rounded) != 0) return -1;
    *out = rounded & ~(alignment - 1u);
    return 0;
}

int jz_size_grow_doubling_checked(size_t current,
                                  size_t minimum,
                                  size_t initial,
                                  size_t *out)
{
    size_t grown = 0;

    if (!out) return -1;
    if (minimum == 0) {
        *out = current;
        return 0;
    }

    grown = current ? current : initial;
    if (grown == 0) {
        grown = 1;
    }
    while (grown < minimum) {
        size_t next = 0;
        if (grown > SIZE_MAX / 2) {
            grown = minimum;
            break;
        }
        if (jz_size_add_checked(grown, grown, &next) != 0) {
            grown = minimum;
            break;
        }
        grown = next;
    }
    if (grown < minimum) {
        return -1;
    }
    *out = grown;
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

int jz_get_fp_size(FILE *fp, size_t *out_size)
{
    long cur = 0;
    long end = 0;

    if (!fp || !out_size) return -1;
    if ((cur = ftell(fp)) < 0) return -1;
    if (fseek(fp, 0, SEEK_END) != 0) return -1;
    end = ftell(fp);
    if (end < 0) {
        (void)fseek(fp, cur, SEEK_SET);
        return -1;
    }
    if (fseek(fp, cur, SEEK_SET) != 0) return -1;
    *out_size = (size_t)end;
    return 0;
}

int jz_get_file_size(const char *filename, size_t *out_size)
{
    FILE *f = NULL;
    int rc = -1;

    if (!filename || !out_size) return -1;
    f = fopen(filename, "rb");
    if (!f) return -1;
    rc = jz_get_fp_size(f, out_size);
    fclose(f);
    return rc;
}

char *jz_read_entire_fp_limit(FILE *fp, size_t max_size, size_t *out_size)
{
    size_t len = 0;
    size_t alloc_size = 0;
    char *buf = NULL;
    size_t read = 0;

    if (!fp) return NULL;
    if (jz_get_fp_size(fp, &len) != 0) return NULL;
    if (len > max_size) return NULL;
    if (fseek(fp, 0, SEEK_SET) != 0) return NULL;
    if (jz_size_add_checked(len, 1, &alloc_size) != 0) return NULL;

    buf = (char *)malloc(alloc_size);
    if (!buf) return NULL;

    read = fread(buf, 1, len, fp);
    if (read != len) {
        free(buf);
        return NULL;
    }

    buf[len] = '\0';
    if (out_size) *out_size = len;
    return buf;
}

char *jz_read_entire_file_limit(const char *filename, size_t max_size, size_t *out_size)
{
    FILE *f = fopen(filename, "rb");
    char *buf = NULL;
    if (!f) return NULL;
    buf = jz_read_entire_fp_limit(f, max_size, out_size);
    fclose(f);
    return buf;
}

char *jz_read_entire_file(const char *filename, size_t *out_size) {
    FILE *f = fopen(filename, "rb");
    char *buf = NULL;
    if (!f) return NULL;
    buf = jz_read_entire_fp_limit(f, SIZE_MAX - 1u, out_size);
    fclose(f);
    return buf;
}

int jz_buf_reserve(JZBuffer *buf, size_t new_cap)
{
    size_t cap = 0;
    size_t new_bytes = 0;
    unsigned char *data = NULL;

    if (!buf) return -1;
    if (new_cap <= buf->cap) return 0;
    if (jz_size_grow_doubling_checked(buf->cap, new_cap, 16, &cap) != 0) {
        return -1;
    }
    if (jz_size_mul_checked(cap, sizeof(*data), &new_bytes) != 0) {
        return -1;
    }
    data = (unsigned char *)realloc(buf->data, new_bytes);
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

#ifndef _WIN32
static int jz_open_exclusive_fd(const char *path)
{
    int flags = O_CREAT | O_EXCL | O_WRONLY;
#ifdef O_CLOEXEC
    flags |= O_CLOEXEC;
#endif
#ifdef O_NOFOLLOW
    flags |= O_NOFOLLOW;
#endif
    return open(path, flags, 0600);
}

static int jz_build_temp_path(const char *target,
                              unsigned attempt,
                              char *tmp_path,
                              size_t tmp_path_size)
{
    const char *slash = NULL;
    const char *base = NULL;
    int pid = (int)getpid();
    int n = 0;

    if (!target || !tmp_path || tmp_path_size == 0) return -1;
    slash = strrchr(target, '/');
    base = slash ? slash + 1 : target;

    if (slash) {
        size_t dir_len = (size_t)(slash - target);
        n = snprintf(tmp_path, tmp_path_size,
                     "%.*s/.%s.tmp.%d.%u",
                     (int)dir_len, target, base, pid, attempt);
    } else {
        n = snprintf(tmp_path, tmp_path_size,
                     ".%s.tmp.%d.%u",
                     base, pid, attempt);
    }
    if (n <= 0 || (size_t)n >= tmp_path_size) return -1;
    return 0;
}

int jz_open_exclusive_temp_output(const char *target,
                                  FILE **out,
                                  char *tmp_path,
                                  size_t tmp_path_size)
{
    unsigned attempt = 0;

    if (!target || !out || !tmp_path || tmp_path_size == 0) return -1;
    *out = NULL;
    tmp_path[0] = '\0';

    for (attempt = 0; attempt < 256u; ++attempt) {
        int fd = -1;
        if (jz_build_temp_path(target, attempt, tmp_path, tmp_path_size) != 0) {
            return -1;
        }
        fd = jz_open_exclusive_fd(tmp_path);
        if (fd < 0) {
            if (errno == EEXIST) continue;
            tmp_path[0] = '\0';
            return -1;
        }
        *out = fdopen(fd, "w");
        if (!*out) {
            close(fd);
            (void)unlink(tmp_path);
            tmp_path[0] = '\0';
            return -1;
        }
        return 0;
    }

    tmp_path[0] = '\0';
    return -1;
}

int jz_commit_exclusive_temp_output(FILE *out,
                                    const char *tmp_path,
                                    const char *final_path)
{
    if (!out || !tmp_path || !final_path || final_path[0] == '\0') return -1;
    if (fflush(out) != 0 || ferror(out)) {
        fclose(out);
        (void)unlink(tmp_path);
        return -1;
    }
    if (fclose(out) != 0) {
        (void)unlink(tmp_path);
        return -1;
    }
    if (rename(tmp_path, final_path) != 0) {
        (void)unlink(tmp_path);
        return -1;
    }
    return 0;
}

int jz_open_unique_sidecar_output(const char *prefix,
                                  const char *suffix,
                                  FILE **out,
                                  char *path_buf,
                                  size_t path_buf_size)
{
    unsigned attempt = 0;
    int pid = (int)getpid();

    if (!prefix || !suffix || !out || !path_buf || path_buf_size == 0) return -1;
    *out = NULL;
    path_buf[0] = '\0';

    for (attempt = 0; attempt < 256u; ++attempt) {
        int fd = -1;
        int n = snprintf(path_buf, path_buf_size, "%s.%d.%u%s",
                         prefix, pid, attempt, suffix);
        if (n <= 0 || (size_t)n >= path_buf_size) {
            path_buf[0] = '\0';
            return -1;
        }
        fd = jz_open_exclusive_fd(path_buf);
        if (fd < 0) {
            if (errno == EEXIST) continue;
            path_buf[0] = '\0';
            return -1;
        }
        *out = fdopen(fd, "w");
        if (!*out) {
            close(fd);
            (void)unlink(path_buf);
            path_buf[0] = '\0';
            return -1;
        }
        return 0;
    }

    path_buf[0] = '\0';
    return -1;
}
#else
int jz_open_exclusive_temp_output(const char *target,
                                  FILE **out,
                                  char *tmp_path,
                                  size_t tmp_path_size)
{
    (void)target;
    (void)out;
    (void)tmp_path;
    (void)tmp_path_size;
    return -1;
}

int jz_commit_exclusive_temp_output(FILE *out,
                                    const char *tmp_path,
                                    const char *final_path)
{
    (void)out;
    (void)tmp_path;
    (void)final_path;
    return -1;
}

int jz_open_unique_sidecar_output(const char *prefix,
                                  const char *suffix,
                                  FILE **out,
                                  char *path_buf,
                                  size_t path_buf_size)
{
    (void)prefix;
    (void)suffix;
    (void)out;
    (void)path_buf;
    (void)path_buf_size;
    return -1;
}
#endif
