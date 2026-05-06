/**
 * @file driver_mem.c
 * @brief Semantic checks for MEM declarations, initializers, and access rules.
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>

#include "sem_driver.h"
#include "sem.h"
#include "driver_internal.h"
#include "chip_data.h"
#include "path_security.h"
#include "../parser/parser_internal.h"

/* Provided by driver.c; used here to enforce x-free MEM initialization
 * literals consistent with the Observability Rule and literal semantics.
 */
int sem_literal_has_x_bits(const char *lex);
int sem_literal_has_z_bits(const char *lex);
#include "rules.h"
#include "driver_internal.h"

/**
 * @brief Validate an aggregate-style MUX declaration.
 * @param mux_decl MUX declaration to validate.
 * @param scope Module scope containing the declaration.
 * @param diagnostics Diagnostic sink for reported issues.
 */
static void sem_check_mux_aggregate_decl(JZASTNode *mux_decl,
                                         const JZModuleScope *scope,
                                         JZDiagnosticList *diagnostics);
/**
 * @brief Validate a slice-style MUX declaration.
 * @param mux_decl MUX declaration to validate.
 * @param scope Module scope containing the declaration.
 * @param diagnostics Diagnostic sink for reported issues.
 */
static void sem_check_mux_slice_decl(JZASTNode *mux_decl,
                                     const JZModuleScope *scope,
                                     JZDiagnosticList *diagnostics);

/* sem_extract_identifier_like: now shared from driver.c via driver_internal.h */

/**
 * @brief Heuristically distinguish MEM file-path initializers from numeric literals.
 * @param s Literal text to inspect.
 * @return Non-zero when the text looks like a file path, or zero otherwise.
 */
static int sem_mem_init_looks_like_file_path(const char *s)
{
    if (!s || !*s) return 0;
    for (const char *p = s; *p; ++p) {
        /* Directory separators or dots strongly indicate a path/filename. */
        if (*p == '/' || *p == '\\' || *p == '.') return 1;
        /* Whitespace inside the literal is also not valid for numeric forms. */
        if (isspace((unsigned char)*p)) return 1;
    }
    return 0;
}

/* Build a filesystem path for a MEM @file initializer. If the initializer
 * path already contains a directory component, use it as-is. Otherwise,
 * treat it as relative to the directory of the source file that declared
 * the MEM.
 */
/* Extract a case-insensitive file extension from a path, ignoring any
 * directory components. Returns a pointer into the original string, or
 * NULL if no extension is present.
 */
static const char *sem_mem_get_file_ext(const char *path)
{
    if (!path) return NULL;

    const char *last_slash = strrchr(path, '/');
#ifdef _WIN32
    const char *last_bslash = strrchr(path, '\\');
    if (!last_slash || (last_bslash && last_bslash > last_slash)) {
        last_slash = last_bslash;
    }
#endif
    const char *name = last_slash ? last_slash + 1 : path;
    const char *dot = strrchr(name, '.');
    if (!dot || !*(dot + 1)) {
        return NULL;
    }
    return dot + 1;
}

/* Case-insensitive equality for short ASCII extensions. */
static int sem_mem_ext_equals(const char *ext, const char *want)
{
    if (!ext || !want) return 0;
    while (*ext && *want) {
        char c1 = (char)tolower((unsigned char)*ext);
        char c2 = (char)tolower((unsigned char)*want);
        if (c1 != c2) return 0;
        ++ext;
        ++want;
    }
    return *ext == '\0' && *want == '\0';
}

static void sem_base_dir_from_filename_resolved(const char *filename,
                                                char *buf,
                                                size_t bufsz)
{
    const char *path = filename;
    buf[0] = '\0';
    if (!path || !*path || bufsz == 0) return;

    if (!strchr(path, '/')) {
        size_t count = g_imported_filenames_len;
        if (g_imported_resolved_paths_len < count) {
            count = g_imported_resolved_paths_len;
        }
        for (size_t i = 0; i < count; ++i) {
            if (g_imported_filenames[i] &&
                strcmp(g_imported_filenames[i], path) == 0 &&
                g_imported_resolved_paths[i] &&
                *g_imported_resolved_paths[i]) {
                path = g_imported_resolved_paths[i];
                break;
            }
        }
    }

    const char *slash = strrchr(path, '/');
    if (!slash) return;

    size_t dir_len = (size_t)(slash - path);
    if (dir_len >= bufsz) dir_len = bufsz - 1;
    memcpy(buf, path, dir_len);
    buf[dir_len] = '\0';
}

/* Count logical bits in a hex text file: 0-9, A-F, a-f are treated as
 * hexadecimal digits contributing 4 bits each. Underscores and whitespace
 * (including newlines) are ignored; any other characters are skipped.
 */
static unsigned long long sem_mem_count_bits_hex_file(FILE *fp)
{
    unsigned long long bits = 0ull;
    int ch;
    while ((ch = fgetc(fp)) != EOF) {
        if (ch == '_' || isspace((unsigned char)ch)) {
            continue;
        }
        if ((ch >= '0' && ch <= '9') ||
            (ch >= 'a' && ch <= 'f') ||
            (ch >= 'A' && ch <= 'F')) {
            bits += 4ull;
        }
    }
    return bits;
}

/* Count logical bits in a binary-text MEM file: '0' and '1' are treated as
 * individual bits, underscores and whitespace (including newlines) are
 * ignored; any other characters are skipped.
 */
static unsigned long long sem_mem_count_bits_mem_file(FILE *fp)
{
    unsigned long long bits = 0ull;
    int ch;
    while ((ch = fgetc(fp)) != EOF) {
        if (ch == '_' || isspace((unsigned char)ch)) {
            continue;
        }
        if (ch == '0' || ch == '1') {
            bits += 1ull;
        }
    }
    return bits;
}

/** @brief Radix modes recognized while parsing MEM initialization files. */
typedef enum {
    SEM_MEM_RADIX_NONE = 0, /**< No radix specified. */
    SEM_MEM_RADIX_BIN = 2,  /**< Binary radix. */
    SEM_MEM_RADIX_OCT = 8,  /**< Octal radix. */
    SEM_MEM_RADIX_DEC = 10, /**< Decimal radix. */
    SEM_MEM_RADIX_HEX = 16, /**< Hexadecimal radix. */
    SEM_MEM_RADIX_UNS = 100 /**< Unspecified radix marker. */
} SemMemRadix;

static int sem_mem_ci_char_eq(char a, char b)
{
    return tolower((unsigned char)a) == tolower((unsigned char)b);
}

static int sem_mem_ci_prefix_eq(const char *text, const char *prefix)
{
    while (*text && *prefix) {
        if (!sem_mem_ci_char_eq(*text, *prefix)) return 0;
        ++text;
        ++prefix;
    }
    return *prefix == '\0';
}

static int sem_mem_is_ident_char(char ch)
{
    return isalnum((unsigned char)ch) || ch == '_';
}

static const char *sem_mem_find_keyword_ci(const char *text, const char *keyword)
{
    size_t key_len;
    if (!text || !keyword) return NULL;
    key_len = strlen(keyword);
    if (key_len == 0) return NULL;

    for (const char *p = text; *p; ++p) {
        if ((p == text || !sem_mem_is_ident_char(p[-1])) &&
            sem_mem_ci_prefix_eq(p, keyword) &&
            !sem_mem_is_ident_char(p[key_len])) {
            return p;
        }
    }
    return NULL;
}

static void sem_mem_trim_span(const char **start, const char **end)
{
    while (*start < *end && isspace((unsigned char)**start)) {
        (*start)++;
    }
    while (*end > *start && isspace((unsigned char)(*end)[-1])) {
        (*end)--;
    }
}

static char *sem_mem_dup_span(const char *start, const char *end)
{
    size_t len;
    char *copy;
    if (!start || !end || end < start) return NULL;
    len = (size_t)(end - start);
    copy = (char *)malloc(len + 1);
    if (!copy) return NULL;
    memcpy(copy, start, len);
    copy[len] = '\0';
    return copy;
}

static int sem_mem_extract_assignment(const char *text,
                                      const char *keyword,
                                      const char **value_start,
                                      const char **value_end)
{
    size_t key_len;
    const char *stmt;
    if (!text || !keyword || !value_start || !value_end) return -1;
    *value_start = NULL;
    *value_end = NULL;
    key_len = strlen(keyword);

    stmt = text;
    while (*stmt) {
        const char *stmt_end = strchr(stmt, ';');
        const char *lhs_start;
        const char *lhs_end;
        const char *rhs_start;
        const char *rhs_end;
        const char *eq;

        if (!stmt_end) stmt_end = stmt + strlen(stmt);
        lhs_start = stmt;
        lhs_end = stmt_end;
        sem_mem_trim_span(&lhs_start, &lhs_end);
        if (lhs_start < lhs_end) {
            eq = memchr(lhs_start, '=', (size_t)(lhs_end - lhs_start));
            if (eq) {
                lhs_end = eq;
                rhs_start = eq + 1;
                rhs_end = stmt_end;
                sem_mem_trim_span(&lhs_start, &lhs_end);
                sem_mem_trim_span(&rhs_start, &rhs_end);
                if ((size_t)(lhs_end - lhs_start) == key_len &&
                    sem_mem_ci_prefix_eq(lhs_start, keyword)) {
                    *value_start = rhs_start;
                    *value_end = rhs_end;
                    return 0;
                }
            }
        }
        stmt = *stmt_end ? stmt_end + 1 : stmt_end;
    }
    return -1;
}

static char *sem_mem_read_entire_fp(FILE *fp, size_t *size_out)
{
    if (!fp) return NULL;
    return jz_read_entire_fp_limit(fp,
                                   jz_input_limit_value(JZ_LIMIT_MEM_INIT_FILE_BYTES),
                                   size_out);
}

static char *sem_mem_strip_mif_comments(const char *contents, size_t size)
{
    char *out = (char *)malloc(size + 1);
    size_t in = 0;
    size_t out_len = 0;
    if (!out) return NULL;

    while (in < size) {
        if (contents[in] == '%') {
            ++in;
            while (in < size && contents[in] != '%') {
                ++in;
            }
            if (in < size) ++in;
            continue;
        }
        if (contents[in] == '-' && in + 1 < size && contents[in + 1] == '-') {
            in += 2;
            while (in < size && contents[in] != '\n' && contents[in] != '\r') {
                ++in;
            }
            continue;
        }
        out[out_len++] = contents[in++];
    }

    out[out_len] = '\0';
    return out;
}

static int sem_mem_token_has_xz(const char *start, const char *end)
{
    for (const char *p = start; p < end; ++p) {
        if (*p == 'x' || *p == 'X' || *p == 'z' || *p == 'Z') {
            return 1;
        }
    }
    return 0;
}

static int sem_mem_parse_radix_string(const char *start,
                                      const char *end,
                                      SemMemRadix *out_radix)
{
    char *copy;
    SemMemRadix radix = SEM_MEM_RADIX_NONE;
    if (!start || !end || !out_radix) return -1;
    copy = sem_mem_dup_span(start, end);
    if (!copy) return -1;

    if (sem_mem_ci_prefix_eq(copy, "BIN") && strlen(copy) == 3) {
        radix = SEM_MEM_RADIX_BIN;
    } else if (sem_mem_ci_prefix_eq(copy, "OCT") && strlen(copy) == 3) {
        radix = SEM_MEM_RADIX_OCT;
    } else if (sem_mem_ci_prefix_eq(copy, "DEC") && strlen(copy) == 3) {
        radix = SEM_MEM_RADIX_DEC;
    } else if (sem_mem_ci_prefix_eq(copy, "HEX") && strlen(copy) == 3) {
        radix = SEM_MEM_RADIX_HEX;
    } else if (sem_mem_ci_prefix_eq(copy, "UNS") && strlen(copy) == 3) {
        radix = SEM_MEM_RADIX_UNS;
    }

    free(copy);
    if (radix == SEM_MEM_RADIX_NONE) return -1;
    *out_radix = radix;
    return 0;
}

static int sem_mem_parse_coe_radix(const char *start,
                                   const char *end,
                                   SemMemRadix *out_radix)
{
    char *copy;
    if (!start || !end || !out_radix) return -1;
    copy = sem_mem_dup_span(start, end);
    if (!copy) return -1;

    if (strcmp(copy, "2") == 0) {
        *out_radix = SEM_MEM_RADIX_BIN;
    } else if (strcmp(copy, "10") == 0) {
        *out_radix = SEM_MEM_RADIX_DEC;
    } else if (strcmp(copy, "16") == 0) {
        *out_radix = SEM_MEM_RADIX_HEX;
    } else {
        free(copy);
        return -1;
    }

    free(copy);
    return 0;
}

static int sem_mem_parse_unsigned_value(const char *start,
                                        const char *end,
                                        SemMemRadix radix,
                                        unsigned long long *out_value)
{
    char *copy;
    char *clean;
    char *dst;
    char *endptr;
    int base;
    unsigned long long value;

    if (!start || !end || !out_value) return -1;
    copy = sem_mem_dup_span(start, end);
    if (!copy) return -1;
    clean = (char *)malloc(strlen(copy) + 1);
    if (!clean) {
        free(copy);
        return -1;
    }

    dst = clean;
    for (const char *src = copy; *src; ++src) {
        if (*src != '_') {
            *dst++ = *src;
        }
    }
    *dst = '\0';
    free(copy);

    if (clean[0] == '\0' || clean[0] == '-') {
        free(clean);
        return -1;
    }

    if (radix == SEM_MEM_RADIX_BIN) base = 2;
    else if (radix == SEM_MEM_RADIX_OCT) base = 8;
    else if (radix == SEM_MEM_RADIX_HEX) base = 16;
    else if (radix == SEM_MEM_RADIX_DEC || radix == SEM_MEM_RADIX_UNS) base = 10;
    else {
        free(clean);
        return -1;
    }

    errno = 0;
    value = strtoull(clean, &endptr, base);
    if (errno == ERANGE || !endptr || *endptr != '\0') {
        free(clean);
        return -1;
    }
    free(clean);
    *out_value = value;
    return 0;
}

static void sem_mem_mark_addr(unsigned char *assigned,
                              unsigned depth,
                              unsigned long long addr,
                              unsigned long long *count,
                              int *overflow)
{
    if (addr >= (unsigned long long)depth) {
        *overflow = 1;
        return;
    }
    if (!assigned[addr]) {
        assigned[addr] = 1;
        (*count)++;
    }
}

static int sem_mem_is_saturated_mif_addr(unsigned long long addr)
{
    return addr == ULLONG_MAX;
}

static void sem_mem_mark_addr_range(unsigned char *assigned,
                                    unsigned depth,
                                    unsigned long long start_addr,
                                    unsigned long long end_addr,
                                    unsigned long long *count,
                                    int *overflow)
{
    unsigned long long capped_end;

    if (start_addr >= (unsigned long long)depth) {
        *overflow = 1;
        return;
    }

    capped_end = end_addr;
    if (capped_end >= (unsigned long long)depth) {
        capped_end = (unsigned long long)depth - 1ull;
        *overflow = 1;
    }

    for (unsigned long long addr = start_addr; addr <= capped_end; ++addr) {
        sem_mem_mark_addr(assigned, depth, addr, count, overflow);
    }
}

static void sem_mem_mark_addr_sequence(unsigned char *assigned,
                                       unsigned depth,
                                       unsigned long long start_addr,
                                       int token_count,
                                       unsigned long long *count,
                                       int *overflow)
{
    unsigned long long available;
    int mark_count;

    if (token_count <= 0) return;
    if (start_addr >= (unsigned long long)depth) {
        *overflow = 1;
        return;
    }

    available = (unsigned long long)depth - start_addr;
    mark_count = token_count;
    if ((unsigned long long)mark_count > available) {
        mark_count = (int)available;
        *overflow = 1;
    }

    for (int idx = 0; idx < mark_count; ++idx) {
        sem_mem_mark_addr(assigned, depth, start_addr + (unsigned long long)idx,
                          count, overflow);
    }
}

static int sem_mem_count_bits_coe_file(FILE *fp,
                                       unsigned word_w,
                                       unsigned long long *file_bits,
                                       int *found_xz)
{
    size_t size = 0;
    char *contents = sem_mem_read_entire_fp(fp, &size);
    const char *radix_start;
    const char *radix_end;
    const char *vec_start;
    const char *vec_end;
    SemMemRadix radix;
    unsigned long long words = 0ull;

    if (!contents) return -1;

    if (memset(found_xz, 0, sizeof(*found_xz)),
        sem_mem_extract_assignment(contents, "memory_initialization_radix",
                                   &radix_start, &radix_end) != 0 ||
        sem_mem_parse_coe_radix(radix_start, radix_end, &radix) != 0 ||
        sem_mem_extract_assignment(contents, "memory_initialization_vector",
                                   &vec_start, &vec_end) != 0) {
        free(contents);
        return -1;
    }

    (void)radix;
    while (vec_start < vec_end) {
        const char *tok_start;
        const char *tok_end;
        while (vec_start < vec_end &&
               (isspace((unsigned char)*vec_start) || *vec_start == ',')) {
            ++vec_start;
        }
        tok_start = vec_start;
        while (vec_start < vec_end &&
               !isspace((unsigned char)*vec_start) &&
               *vec_start != ',') {
            ++vec_start;
        }
        tok_end = vec_start;
        if (tok_start == tok_end) continue;
        if (sem_mem_token_has_xz(tok_start, tok_end)) {
            *found_xz = 1;
        }
        ++words;
    }

    free(contents);
    *file_bits = words * (unsigned long long)word_w;
    return 0;
}

static int sem_mem_count_bits_mif_file(FILE *fp,
                                       unsigned word_w,
                                       unsigned depth,
                                       unsigned long long *file_bits,
                                       int *found_xz)
{
    size_t size = 0;
    char *contents = sem_mem_read_entire_fp(fp, &size);
    char *stripped = NULL;
    const char *addr_radix_start;
    const char *addr_radix_end;
    const char *data_radix_start;
    const char *data_radix_end;
    const char *content_kw;
    const char *begin_kw;
    const char *end_kw;
    SemMemRadix addr_radix = SEM_MEM_RADIX_NONE;
    SemMemRadix data_radix = SEM_MEM_RADIX_NONE;
    unsigned char *assigned = NULL;
    unsigned long long assigned_words = 0ull;
    int overflow = 0;

    if (depth > JZ_MAX_MEM_INIT_MIF_DEPTH) return -1;
    if (!contents) return -1;
    stripped = sem_mem_strip_mif_comments(contents, size);
    free(contents);
    if (!stripped) return -1;

    if (sem_mem_extract_assignment(stripped, "ADDRESS_RADIX",
                                   &addr_radix_start, &addr_radix_end) != 0 ||
        sem_mem_parse_radix_string(addr_radix_start, addr_radix_end,
                                   &addr_radix) != 0 ||
        sem_mem_extract_assignment(stripped, "DATA_RADIX",
                                   &data_radix_start, &data_radix_end) != 0 ||
        sem_mem_parse_radix_string(data_radix_start, data_radix_end,
                                   &data_radix) != 0) {
        free(stripped);
        return -1;
    }

    content_kw = sem_mem_find_keyword_ci(stripped, "CONTENT");
    begin_kw = content_kw ? sem_mem_find_keyword_ci(content_kw, "BEGIN") : NULL;
    end_kw = begin_kw ? sem_mem_find_keyword_ci(begin_kw, "END") : NULL;
    if (!content_kw || !begin_kw || !end_kw || end_kw <= begin_kw) {
        free(stripped);
        return -1;
    }

    assigned = (unsigned char *)calloc(depth > 0 ? depth : 1u, 1);
    if (!assigned) {
        free(stripped);
        return -1;
    }
    *found_xz = 0;
    (void)data_radix;

    {
        const char *cursor = begin_kw + strlen("BEGIN");
        while (cursor < end_kw) {
            const char *stmt_end = strchr(cursor, ';');
            const char *entry_start;
            const char *entry_end;
            const char *colon;
            const char *addr_spec_start;
            const char *addr_spec_end;
            const char *data_spec_start;
            const char *data_spec_end;
            unsigned long long start_addr = 0;
            unsigned long long end_addr = 0;
            int is_range = 0;
            int token_count = 0;

            if (!stmt_end || stmt_end > end_kw) break;
            entry_start = cursor;
            entry_end = stmt_end;
            cursor = stmt_end + 1;
            sem_mem_trim_span(&entry_start, &entry_end);
            if (entry_start >= entry_end) continue;

            colon = memchr(entry_start, ':', (size_t)(entry_end - entry_start));
            if (!colon) continue;

            addr_spec_start = entry_start;
            addr_spec_end = colon;
            data_spec_start = colon + 1;
            data_spec_end = entry_end;
            sem_mem_trim_span(&addr_spec_start, &addr_spec_end);
            sem_mem_trim_span(&data_spec_start, &data_spec_end);

            if (addr_spec_start < addr_spec_end && *addr_spec_start == '[') {
                const char *inner_start = addr_spec_start + 1;
                const char *inner_end = addr_spec_end;
                const char *dots = NULL;
                if (addr_spec_end <= addr_spec_start + 2 || addr_spec_end[-1] != ']') {
                    free(assigned);
                    free(stripped);
                    return -1;
                }
                inner_end--;
                for (const char *p = inner_start; p + 1 < inner_end; ++p) {
                    if (p[0] == '.' && p[1] == '.') {
                        dots = p;
                        break;
                    }
                }
                if (!dots) {
                    free(assigned);
                    free(stripped);
                    return -1;
                }
                {
                    const char *lhs_start = inner_start;
                    const char *lhs_end = dots;
                    const char *rhs_start = dots + 2;
                    const char *rhs_end = inner_end;
                    sem_mem_trim_span(&lhs_start, &lhs_end);
                    sem_mem_trim_span(&rhs_start, &rhs_end);
                    if (sem_mem_parse_unsigned_value(lhs_start, lhs_end, addr_radix, &start_addr) != 0 ||
                        sem_mem_parse_unsigned_value(rhs_start, rhs_end, addr_radix, &end_addr) != 0 ||
                        sem_mem_is_saturated_mif_addr(start_addr) ||
                        sem_mem_is_saturated_mif_addr(end_addr) ||
                        end_addr < start_addr) {
                        free(assigned);
                        free(stripped);
                        return -1;
                    }
                }
                is_range = 1;
            } else {
                if (sem_mem_parse_unsigned_value(addr_spec_start, addr_spec_end,
                                                 addr_radix, &start_addr) != 0) {
                    free(assigned);
                    free(stripped);
                    return -1;
                }
                if (sem_mem_is_saturated_mif_addr(start_addr)) {
                    free(assigned);
                    free(stripped);
                    return -1;
                }
                end_addr = start_addr;
            }

            {
                const char *p = data_spec_start;
                while (p <= data_spec_end) {
                    const char *tok_start = NULL;
                    const char *tok_end = NULL;
                    while (p < data_spec_end &&
                           (isspace((unsigned char)*p) || *p == ',')) {
                        ++p;
                    }
                    tok_start = p;
                    while (p < data_spec_end &&
                           !isspace((unsigned char)*p) &&
                           *p != ',') {
                        ++p;
                    }
                    tok_end = p;
                    if (tok_start < tok_end) {
                        if (sem_mem_token_has_xz(tok_start, tok_end)) {
                            *found_xz = 1;
                        }
                        ++token_count;
                    }
                    if (p == data_spec_end) break;
                    ++p;
                }
            }

            if (token_count <= 0) {
                free(assigned);
                free(stripped);
                return -1;
            }

            if (is_range) {
                sem_mem_mark_addr_range(assigned, depth, start_addr, end_addr,
                                        &assigned_words, &overflow);
            } else {
                sem_mem_mark_addr_sequence(assigned, depth, start_addr,
                                           token_count, &assigned_words,
                                           &overflow);
            }
        }
    }

    free(assigned);
    free(stripped);
    if (overflow) {
        *file_bits = ((unsigned long long)depth + 1ull) * (unsigned long long)word_w;
    } else {
        *file_bits = assigned_words * (unsigned long long)word_w;
    }
    return 0;
}

/* Validate a file-based MEM initializer against the declared word width and
 * depth, emitting MEM_INIT_FILE_NOT_FOUND, MEM_INIT_FILE_TOO_LARGE, or
 * MEM_WARN_PARTIAL_INIT as appropriate.
 *
 * File semantics are selected based on the (case-insensitive) file
 * extension of the @file payload:
 *   - .hex : Base-16 text (0-9, A-F, _ and whitespace ignored);
 *            each hex digit contributes 4 bits.
 *   - .mem : Base-2 text (0/1, _ and whitespace ignored);
 *            each binary digit contributes 1 bit.
 *   - .bin : raw binary; file size in bytes is used.
 *   - .mif : Intel/Altera memory initialization file.
 *   - .coe : Xilinx/AMD coefficient memory init file.
 *   - anything else : raw binary; file size in bytes is used.
 */
static void sem_check_mem_file_init(JZASTNode *mem,
                                    JZASTNode *init_expr,
                                    unsigned word_w,
                                    unsigned depth,
                                    int have_word_w,
                                    int have_depth,
                                    JZDiagnosticList *diagnostics)
{
    if (!mem || !init_expr || !init_expr->text || !diagnostics) return;

    /* Validate the @file() path against security policy. */
    char base_dir[512];
    sem_base_dir_from_filename_resolved(mem->loc.filename,
                                        base_dir,
                                        sizeof(base_dir));

    char *validated = jz_path_validate(init_expr->text,
                                        base_dir[0] ? base_dir : NULL,
                                        init_expr->loc,
                                        diagnostics);
    char fullpath[512];
    size_t file_size = 0;
    if (validated) {
        snprintf(fullpath, sizeof(fullpath), "%s", validated);
        free(validated);
    } else {
        /* Path validation failed; diagnostic already emitted. */
        return;
    }

    if (jz_get_file_size(fullpath, &file_size) == 0 &&
        file_size > jz_input_limit_value(JZ_LIMIT_MEM_INIT_FILE_BYTES)) {
        char msg[640];
        snprintf(msg, sizeof(msg),
                 "MEM init file '%s' is %zu byte(s), exceeding the compiler safety limit of %u byte(s)",
                 fullpath,
                 file_size,
                 (unsigned)jz_input_limit_value(JZ_LIMIT_MEM_INIT_FILE_BYTES));
        sem_report_rule(diagnostics,
                        init_expr->loc,
                        "MEM_INIT_FILE_HARD_LIMIT_EXCEEDED",
                        msg);
        return;
    }

    FILE *fp = fopen(fullpath, "rb");
    if (!fp) {
        char msg[600];
        snprintf(msg, sizeof(msg),
                 "MEM init file not found or not readable: %s", fullpath);
        sem_report_rule(diagnostics,
                        init_expr->loc,
                        "MEM_INIT_FILE_NOT_FOUND",
                        msg);
        return;
    }

    if (!have_word_w || !have_depth || word_w == 0 || depth == 0) {
        /* We validated existence but cannot reason about size without
         * simple literal width/depth information.
         */
        fclose(fp);
        return;
    }

    const char *ext = sem_mem_get_file_ext(init_expr->text);
    unsigned long long file_bits = 0ull;

    if (ext && sem_mem_ext_equals(ext, "mif") &&
        depth > jz_input_limit_value(JZ_LIMIT_MEM_INIT_MIF_DEPTH)) {
        char msg[640];
        fclose(fp);
        snprintf(msg, sizeof(msg),
                 "MEM declared depth %u exceeds the compiler MIF safety limit of %u words",
                 depth, (unsigned)jz_input_limit_value(JZ_LIMIT_MEM_INIT_MIF_DEPTH));
        sem_report_rule(diagnostics,
                        init_expr->loc,
                        "MEM_INIT_MIF_DEPTH_LIMIT_EXCEEDED",
                        msg);
        return;
    }

    /* MEM_INIT_FILE_CONTAINS_X: scan text-format files for x/z values. */
    if (ext && (sem_mem_ext_equals(ext, "hex") ||
                sem_mem_ext_equals(ext, "mem"))) {
        int ch;
        int found_xz = 0;
        while ((ch = fgetc(fp)) != EOF) {
            if (ch == 'x' || ch == 'X' || ch == 'z' || ch == 'Z') {
                found_xz = 1;
                break;
            }
        }
        if (found_xz) {
            sem_report_rule(diagnostics,
                            init_expr->loc,
                            "MEM_INIT_FILE_CONTAINS_X",
                            "memory initialization file contains x or z values");
        }
        rewind(fp);
    }

    if (ext && sem_mem_ext_equals(ext, "hex")) {
        /* Textual hex: count hex digits as 4 bits each. */
        file_bits = sem_mem_count_bits_hex_file(fp);
        fclose(fp);
    } else if (ext && sem_mem_ext_equals(ext, "mem")) {
        /* Textual binary: count '0'/'1' digits as 1 bit each. */
        file_bits = sem_mem_count_bits_mem_file(fp);
        fclose(fp);
    } else if (ext && sem_mem_ext_equals(ext, "coe")) {
        int found_xz = 0;
        if (sem_mem_count_bits_coe_file(fp, word_w, &file_bits, &found_xz) != 0) {
            fclose(fp);
            return;
        }
        fclose(fp);
        if (found_xz) {
            sem_report_rule(diagnostics,
                            init_expr->loc,
                            "MEM_INIT_FILE_CONTAINS_X",
                            "memory initialization file contains x or z values");
        }
    } else if (ext && sem_mem_ext_equals(ext, "mif")) {
        int found_xz = 0;
        if (sem_mem_count_bits_mif_file(fp, word_w, depth, &file_bits, &found_xz) != 0) {
            fclose(fp);
            return;
        }
        fclose(fp);
        if (found_xz) {
            sem_report_rule(diagnostics,
                            init_expr->loc,
                            "MEM_INIT_FILE_CONTAINS_X",
                            "memory initialization file contains x or z values");
        }
    } else {
        /* Default/binary: capacity is based on raw byte size. */
        if (fseek(fp, 0, SEEK_END) != 0) {
            fclose(fp);
            return;
        }
        long sz = ftell(fp);
        fclose(fp);
        if (sz < 0) {
            return;
        }
        file_bits = (unsigned long long)sz * 8ull;
    }

    unsigned long long capacity_bits =
        (unsigned long long)word_w * (unsigned long long)depth;
    if (capacity_bits == 0) {
        return;
    }

    if (file_bits > capacity_bits) {
        sem_report_rule(diagnostics,
                        init_expr->loc,
                        "MEM_INIT_FILE_TOO_LARGE",
                        "MEM init file larger than MEM depth * word_width");
    } else if (file_bits < capacity_bits) {
        char msg[512];
        unsigned long long file_words = word_w > 0 ? file_bits / word_w : 0;
        snprintf(msg, sizeof(msg),
                 "MEM init file has %llu words but MEM depth is %u; "
                 "remaining words zero-filled",
                 file_words, depth);
        sem_report_rule(diagnostics,
                        init_expr->loc,
                        "MEM_WARN_PARTIAL_INIT",
                        msg);
    }
}

/* Given a qualified name "mem.port" or "mem.port.<field>", locate the
 * corresponding MEM_DECL and MEM_PORT nodes in the module scope. Returns 1 on
 * success, 0 otherwise. When present, <field> must be "addr" or "data".
 */
static int sem_lookup_mem_port_qualified(const char *qualified,
                                         const JZModuleScope *mod_scope,
                                         JZMemPortRef *out)
{
    if (!qualified || !mod_scope) return 0;
    const char *dot = strchr(qualified, '.');
    if (!dot || !*(dot + 1)) {
        return 0;
    }

    char mem_name[256];
    size_t mem_len = (size_t)(dot - qualified);
    if (mem_len == 0 || mem_len >= sizeof(mem_name)) {
        return 0;
    }
    memcpy(mem_name, qualified, mem_len);
    mem_name[mem_len] = '\0';

    const char *port_str = dot + 1;
    const char *field_str = NULL;
    const char *second_dot = strchr(port_str, '.');
    if (second_dot) {
        if (!*(second_dot + 1)) {
            return 0;
        }
        field_str = second_dot + 1;
    }

    const JZSymbol *mem_sym = module_scope_lookup_kind(mod_scope, mem_name, JZ_SYM_MEM);
    if (!mem_sym || !mem_sym->node) {
        return 0;
    }

    JZASTNode *mem_decl = mem_sym->node;
    JZASTNode *found_port = NULL;
    for (size_t i = 0; i < mem_decl->child_count; ++i) {
        JZASTNode *child = mem_decl->children[i];
        if (!child || child->type != JZ_AST_MEM_PORT || !child->name) {
            continue;
        }
        size_t port_len = second_dot ? (size_t)(second_dot - port_str) : strlen(port_str);
        if (port_len > 0 && strlen(child->name) == port_len &&
            strncmp(child->name, port_str, port_len) == 0) {
            found_port = child;
            break;
        }
    }

    if (!found_port) {
        return 0;
    }

    if (out) {
        out->mem_decl = mem_decl;
        out->port = found_port;
        out->field = MEM_PORT_FIELD_NONE;
        if (field_str) {
            if (strcmp(field_str, "addr") == 0) {
                out->field = MEM_PORT_FIELD_ADDR;
            } else if (strcmp(field_str, "data") == 0) {
                out->field = MEM_PORT_FIELD_DATA;
            } else if (strcmp(field_str, "wdata") == 0) {
                out->field = MEM_PORT_FIELD_WDATA;
            } else {
                return 0;
            }
        }
    }
    return 1;
}

/* Match expressions of the form `mem.port` or `mem.port.<field>` represented as
 * an identifier or qualified-identifier node. On success, fills out and
 * returns 1.
 */
int sem_match_mem_port_qualified_ident(JZASTNode *expr,
                                       const JZModuleScope *mod_scope,
                                       JZDiagnosticList *diagnostics,
                                       JZMemPortRef *out)
{
    (void)diagnostics;
    if (!expr || !expr->name ||
        (expr->type != JZ_AST_EXPR_IDENTIFIER &&
         expr->type != JZ_AST_EXPR_QUALIFIED_IDENTIFIER)) {
        return 0;
    }

    const char *qualified = expr->name;
    const char *dot = strchr(qualified, '.');
    if (!dot || dot == qualified) {
        return 0;
    }

    char mem_name[256];
    size_t mem_len = (size_t)(dot - qualified);
    if (mem_len == 0 || mem_len >= sizeof(mem_name)) {
        return 0;
    }
    memcpy(mem_name, qualified, mem_len);
    mem_name[mem_len] = '\0';

    const JZSymbol *mem_sym = module_scope_lookup_kind(mod_scope, mem_name, JZ_SYM_MEM);
    if (!mem_sym) {
        return 0;
    }

    return sem_lookup_mem_port_qualified(qualified, mod_scope, out);
}

/* Match expressions of the form `mem.port[addr]` represented as a SLICE node.
 * On success, fills out->mem_decl/out->port and returns 1.
 */
int sem_match_mem_port_slice(JZASTNode *slice,
                             const JZModuleScope *mod_scope,
                             JZDiagnosticList *diagnostics,
                             JZMemPortRef *out)
{
    if (!slice || slice->type != JZ_AST_EXPR_SLICE || slice->child_count < 3) {
        return 0;
    }
    JZASTNode *base = slice->children[0];
    if (!base || !base->name ||
        (base->type != JZ_AST_EXPR_IDENTIFIER &&
         base->type != JZ_AST_EXPR_QUALIFIED_IDENTIFIER)) {
        return 0;
    }

    /* Base name is of the form "mem_name.port_name" for MEM accesses. If the
     * MEM name itself is not declared in the module, classify this via
     * MEM_UNDEFINED_NAME before attempting finer-grained MEM_ACCESS checks.
     */
    const char *qualified = base->name;
    const char *dot = strchr(qualified, '.');
    if (!dot || dot == qualified) {
        return 0;
    }

    char mem_name[256];
    size_t mem_len = (size_t)(dot - qualified);
    if (mem_len == 0 || mem_len >= sizeof(mem_name)) {
        return 0;
    }
    memcpy(mem_name, qualified, mem_len);
    mem_name[mem_len] = '\0';

    const JZSymbol *mem_sym = module_scope_lookup_kind(mod_scope, mem_name, JZ_SYM_MEM);
    if (!mem_sym) {
        /* If the name refers to a BUS port rather than a MEM, this is not a
         * MEM access at all — return 0 silently so that the caller can handle
         * it via the normal BUS field resolution path.
         */
        const JZSymbol *port_sym = module_scope_lookup_kind(mod_scope, mem_name, JZ_SYM_PORT);
        if (port_sym && port_sym->node && port_sym->node->block_kind &&
            strcmp(port_sym->node->block_kind, "BUS") == 0) {
            return 0;
        }
        if (diagnostics) {
            sem_report_rule(diagnostics,
                            slice->loc,
                            "MEM_UNDEFINED_NAME",
                            "access to MEM name not declared in module");
        }
        return 0;
    }

    if (!sem_lookup_mem_port_qualified(qualified, mod_scope, out)) {
        return 0;
    }
    if (out && out->field != MEM_PORT_FIELD_NONE) {
        return 0;
    }
    return 1;
}

/* Compute depth and address width (ceil(log2(depth))) for a MEM_DECL whose
 * depth expression is a simple positive integer literal. Returns 1 when both
 * depth and addr_width are known, 0 otherwise.
 */
static int sem_mem_compute_depth_and_addr_width(JZASTNode *mem_decl,
                                                unsigned *out_depth,
                                                unsigned *out_addr_width)
{
    if (!mem_decl) return 0;
    const char *depth_text = mem_decl->text;
    unsigned depth = 0;
    int rc = eval_simple_positive_decl_int(depth_text, &depth);
    if (rc != 1) {
        /* rc == 0: complex/unknown; rc == -1: invalid already reported by
         * declaration-phase checks.
         */
        return 0;
    }

    unsigned addr_width = 0;
    if (depth > 1) {
        unsigned v = depth - 1u;
        while (v) {
            addr_width++;
            v >>= 1;
        }
    }
    /* Minimum address width is 1 bit — 0-width vectors are not permitted. */
    if (addr_width == 0) addr_width = 1;

    if (out_depth) *out_depth = depth;
    if (out_addr_width) *out_addr_width = addr_width;
    return 1;
}

static void sem_check_mem_access_expr_impl(JZASTNode *expr,
                                           const JZModuleScope *mod_scope,
                                           const JZBuffer *project_symbols,
                                           JZDiagnosticList *diagnostics)
{
    if (!expr || !mod_scope) return;
    if (expr->type == JZ_AST_EXPR_SLICE && expr->child_count >= 3) {
        JZMemPortRef ref;
        if (sem_match_mem_port_slice(expr, mod_scope, diagnostics, &ref)) {
            if (ref.port && ref.port->block_kind &&
                strcmp(ref.port->block_kind, "OUT") == 0 &&
                ref.port->text && strcmp(ref.port->text, "SYNC") == 0) {
                sem_report_rule(diagnostics,
                                expr->loc,
                                "MEM_SYNC_PORT_INDEXED",
                                "SYNC MEM read ports may not be indexed; use .addr/.data");
            } else {
                unsigned depth = 0, addr_width = 0;
                if (sem_mem_compute_depth_and_addr_width(ref.mem_decl, &depth, &addr_width)) {
                    /* Index expression is the first index node (msb). For [idx] the parser
                     * creates msb/lsb duplicates, so inspecting msb is sufficient.
                     */
                    JZASTNode *msb_node = expr->children[1];
                    if (msb_node) {
                        if (addr_width > 0) {
                            JZBitvecType idx_type;
                            idx_type.width = 0;
                            idx_type.is_signed = 0;
                            infer_expr_type(msb_node, mod_scope, project_symbols, diagnostics, &idx_type);
                            if (idx_type.width > 0 && idx_type.width != addr_width) {
                                sem_report_rule(diagnostics,
                                                expr->loc,
                                                "MEM_ADDR_WIDTH_MISMATCH",
                                                "memory address expression width must equal ceil(log2(depth))");
                                goto recurse_children;
                            }
                        }

                        /* Constant-address out-of-range check when the address already satisfies
                         * the exact-width rule above. This avoids duplicate diagnostics for a
                         * single malformed literal.
                         */
                        if (msb_node->type == JZ_AST_EXPR_LITERAL && msb_node->text && depth > 0) {
                            unsigned idx = 0;
                            if (parse_literal_unsigned_value(msb_node->text, &idx) && idx >= depth) {
                                sem_report_rule(diagnostics,
                                                expr->loc,
                                                "MEM_CONST_ADDR_OUT_OF_RANGE",
                                                "constant memory address index is out of range for declared depth");
                            }
                        }
                    }
                }
            }
        }
    }

recurse_children:
    for (size_t i = 0; i < expr->child_count; ++i) {
        sem_check_mem_access_expr_impl(expr->children[i],
                                       mod_scope,
                                       project_symbols,
                                       diagnostics);
    }
}

/* Expression-level MEM access checks: address width and constant range. */
void sem_check_mem_access_expr(JZASTNode *expr,
                               const JZModuleScope *mod_scope,
                               const JZBuffer *project_symbols,
                               JZDiagnosticList *diagnostics)
{
    sem_check_mem_access_expr_impl(expr, mod_scope, project_symbols, diagnostics);
}

void sem_check_mem_addr_assign(const JZMemPortRef *ref,
                               JZASTNode *addr_expr,
                               const JZModuleScope *mod_scope,
                               const JZBuffer *project_symbols,
                               JZDiagnosticList *diagnostics)
{
    if (!ref || !ref->mem_decl || !addr_expr || !mod_scope) return;

    unsigned depth = 0, addr_width = 0;
    if (!sem_mem_compute_depth_and_addr_width(ref->mem_decl, &depth, &addr_width)) {
        return;
    }

    if (addr_width > 0) {
        JZBitvecType idx_type;
        idx_type.width = 0;
        idx_type.is_signed = 0;
        infer_expr_type(addr_expr, mod_scope, project_symbols, diagnostics, &idx_type);
        if (idx_type.width > 0 && idx_type.width != addr_width) {
            sem_report_rule(diagnostics,
                            addr_expr->loc,
                            "MEM_ADDR_WIDTH_MISMATCH",
                            "memory address expression width must equal ceil(log2(depth))");
            return;
        }
    }

    if (addr_expr->type == JZ_AST_EXPR_LITERAL && addr_expr->text && depth > 0) {
        unsigned idx = 0;
        if (parse_literal_unsigned_value(addr_expr->text, &idx) && idx >= depth) {
            sem_report_rule(diagnostics,
                            addr_expr->loc,
                            "MEM_CONST_ADDR_OUT_OF_RANGE",
                            "constant memory address index is out of range for declared depth");
        }
    }
}

/* Track writes to MEM OUT ports within a single SYNCHRONOUS block so that we
 * can enforce MEM_MULTIPLE_WRITES_SAME_IN.
 */
void sem_track_mem_out_write(JZBuffer *writes,
                             const JZMemPortRef *ref,
                             JZDiagnosticList *diagnostics,
                             JZLocation loc)
{
    if (!writes || !ref || !ref->mem_decl || !ref->port) return;

    size_t count = writes->len / sizeof(JZMemWriteKey);
    JZMemWriteKey *arr = (JZMemWriteKey *)writes->data;
    for (size_t i = 0; i < count; ++i) {
        if (arr[i].mem_decl == ref->mem_decl && arr[i].port == ref->port) {
            sem_report_rule(diagnostics,
                            loc,
                            "MEM_MULTIPLE_WRITES_SAME_IN",
                            "multiple writes to same MEM OUT port within single SYNCHRONOUS block");
            return;
        }
    }

    JZMemWriteKey key;
    key.mem_decl = ref->mem_decl;
    key.port = ref->port;
    (void)jz_buf_append(writes, &key, sizeof(key));
}

/* Helper to recognize MEM(TYPE=...) in the MEM block header attributes,
 * allowing arbitrary whitespace around '=' and case variations for TYPE/values.
 */
/* Return 1 if the attribute string contains a TYPE= key, 0 otherwise.
 * This lets callers distinguish "no TYPE specified" from "TYPE specified but
 * value unrecognized".
 */
static int sem_mem_header_has_type_key(const char *attrs)
{
    if (!attrs) return 0;
    const char *p = strstr(attrs, "TYPE");
    if (!p) p = strstr(attrs, "type");
    if (!p) return 0;
    p += 4;
    while (*p && isspace((unsigned char)*p)) p++;
    return (*p == '=');
}

static JZChipMemType sem_mem_header_parse_type(const char *attrs)
{
    if (!attrs) return JZ_CHIP_MEM_UNKNOWN;
    const char *p = strstr(attrs, "TYPE");
    if (!p) p = strstr(attrs, "type");
    if (!p) return JZ_CHIP_MEM_UNKNOWN;
    p += 4; /* skip TYPE */
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p != '=') return JZ_CHIP_MEM_UNKNOWN;
    p++; /* skip '=' */
    while (*p && isspace((unsigned char)*p)) p++;
    if (strncmp(p, "BLOCK", 5) == 0 || strncmp(p, "block", 5) == 0) {
        return JZ_CHIP_MEM_BLOCK;
    }
    if (strncmp(p, "DISTRIBUTED", 11) == 0 || strncmp(p, "distributed", 11) == 0) {
        return JZ_CHIP_MEM_DISTRIBUTED;
    }
    return JZ_CHIP_MEM_UNKNOWN;
}

void sem_check_module_mem_and_mux_decls(const JZModuleScope *scope,
                                        const JZBuffer *project_symbols,
                                        JZDiagnosticList *diagnostics)
{
    if (!scope || !scope->node) return;
    JZASTNode *mod = scope->node;
    if (mod->type == JZ_AST_BLACKBOX) return;

    for (size_t i = 0; i < mod->child_count; ++i) {
        JZASTNode *child = mod->children[i];
        if (!child) continue;

        if (child->type == JZ_AST_MEM_BLOCK) {
            /* Per-MEM validation: widths/depths, port lists, and TYPE=BLOCK
             * restrictions.
             */
            const char *attrs = child->text;
            JZChipMemType parsed_type = sem_mem_header_parse_type(attrs);
            int type_is_block = (parsed_type == JZ_CHIP_MEM_BLOCK);

            /* MEM_TYPE_INVALID: TYPE= key is present but value is not
             * one of BLOCK or DISTRIBUTED.
             */
            if (sem_mem_header_has_type_key(attrs) &&
                parsed_type == JZ_CHIP_MEM_UNKNOWN) {
                sem_report_rule(diagnostics,
                                child->loc,
                                "MEM_TYPE_INVALID",
                                "MEM TYPE value is not recognized; expected BLOCK or DISTRIBUTED");
            }

            for (size_t j = 0; j < child->child_count; ++j) {
                JZASTNode *mem = child->children[j];
                if (!mem || mem->type != JZ_AST_MEM_DECL) continue;

                /* MEM_INVALID_WORD_WIDTH / MEM_INVALID_DEPTH (simple decimal
                 * forms only; CONST/CONFIG-based expressions are deferred to
                 * future constant-eval integration).
                 */
                unsigned word_w = 0;
                int width_has_lit = (mem->width && sem_expr_has_lit_call(mem->width));
                if (width_has_lit) {
                    sem_report_rule(diagnostics,
                                    mem->loc,
                                    "LIT_INVALID_CONTEXT",
                                    "lit() may not be used in MEM width/depth declarations");
                }
                int w_rc = width_has_lit ? 0 : eval_simple_positive_decl_int(mem->width, &word_w);
                if (w_rc == -1) {
                    sem_report_rule(diagnostics,
                                    mem->loc,
                                    "MEM_INVALID_WORD_WIDTH",
                                    "MEM word width must be a positive integer");
                }

                if (w_rc == 0 && mem->width && !width_has_lit) {
                    int width_config_handled =
                        sem_check_undeclared_config_in_width(mem->width,
                                                             mem->loc,
                                                             project_symbols,
                                                             diagnostics);
                    if (!width_config_handled) {
                        /* If the width expression is a single identifier that
                         * is not a declared CONST or CONFIG, emit the more
                         * specific CONST_UNDEFINED_IN_WIDTH_OR_SLICE rule
                         * instead of the generic MEM_UNDEFINED_CONST_IN_WIDTH.
                         */
                        int width_ident_handled = 0;
                        int width_declared_ident = 0;
                        char width_ident[64];
                        if (sem_extract_identifier_like(mem->width, width_ident, sizeof(width_ident))) {
                            const JZSymbol *wc_sym = module_scope_lookup_kind(scope, width_ident, JZ_SYM_CONST);
                            const JZSymbol *wcfg_sym = NULL;
                            if (project_symbols && project_symbols->data) {
                                const JZSymbol *psyms = (const JZSymbol *)project_symbols->data;
                                size_t pcount = project_symbols->len / sizeof(JZSymbol);
                                for (size_t pi = 0; pi < pcount; ++pi) {
                                    if (psyms[pi].kind == JZ_SYM_CONFIG && psyms[pi].name &&
                                        strcmp(psyms[pi].name, width_ident) == 0) {
                                        wcfg_sym = &psyms[pi];
                                        break;
                                    }
                                }
                            }
                            width_declared_ident = (wc_sym || wcfg_sym) ? 1 : 0;
                            if (!width_declared_ident) {
                                sem_report_rule(diagnostics,
                                                mem->loc,
                                                "CONST_UNDEFINED_IN_WIDTH_OR_SLICE",
                                                "width expression uses undefined CONST/CONFIG name");
                                width_ident_handled = 1;
                            }
                        }

                        if (!width_ident_handled) {
                            /* Try to evaluate as a CONST expression (S7.10 allows
                             * CONST expressions in MEM widths).
                             */
                            long long wval = 0;
                            if (sem_eval_const_expr_in_module(mem->width, scope,
                                                              project_symbols, &wval) != 0) {
                                if (!width_declared_ident) {
                                    sem_report_rule(diagnostics,
                                                    mem->loc,
                                                    "MEM_UNDEFINED_CONST_IN_WIDTH",
                                                    "MEM word width/depth uses undefined CONST name");
                                }
                            } else if (wval <= 0) {
                                sem_report_rule(diagnostics,
                                                mem->loc,
                                                "MEM_INVALID_WORD_WIDTH",
                                                "MEM word width must be a positive integer");
                            }
                        }
                    }
                }

                unsigned depth = 0;
                int depth_has_lit = (mem->text && sem_expr_has_lit_call(mem->text));
                if (depth_has_lit) {
                    sem_report_rule(diagnostics,
                                    mem->loc,
                                    "LIT_INVALID_CONTEXT",
                                    "lit() may not be used in MEM width/depth declarations");
                }
                int d_rc = depth_has_lit ? 0 : eval_simple_positive_decl_int(mem->text, &depth);
                if (d_rc == -1) {
                    sem_report_rule(diagnostics,
                                    mem->loc,
                                    "MEM_INVALID_DEPTH",
                                    "MEM depth must be a positive integer");
                }

                if (d_rc == 0 && mem->text && !depth_has_lit) {
                    int depth_config_handled =
                        sem_check_undeclared_config_in_width(mem->text,
                                                             mem->loc,
                                                             project_symbols,
                                                             diagnostics);
                    if (!depth_config_handled) {
                        /* If the depth expression is a single identifier that
                         * is not a declared CONST or CONFIG, emit the more
                         * specific CONST_UNDEFINED_IN_WIDTH_OR_SLICE rule
                         * instead of the generic MEM_UNDEFINED_CONST_IN_WIDTH.
                         */
                        int depth_ident_handled = 0;
                        int depth_declared_ident = 0;
                        char depth_ident[64];
                        if (sem_extract_identifier_like(mem->text, depth_ident, sizeof(depth_ident))) {
                            const JZSymbol *dc_sym = module_scope_lookup_kind(scope, depth_ident, JZ_SYM_CONST);
                            const JZSymbol *dcfg_sym = NULL;
                            if (project_symbols && project_symbols->data) {
                                const JZSymbol *psyms = (const JZSymbol *)project_symbols->data;
                                size_t pcount = project_symbols->len / sizeof(JZSymbol);
                                for (size_t pi = 0; pi < pcount; ++pi) {
                                    if (psyms[pi].kind == JZ_SYM_CONFIG && psyms[pi].name &&
                                        strcmp(psyms[pi].name, depth_ident) == 0) {
                                        dcfg_sym = &psyms[pi];
                                        break;
                                    }
                                }
                            }
                            depth_declared_ident = (dc_sym || dcfg_sym) ? 1 : 0;
                            if (!depth_declared_ident) {
                                sem_report_rule(diagnostics,
                                                mem->loc,
                                                "CONST_UNDEFINED_IN_WIDTH_OR_SLICE",
                                                "width expression uses undefined CONST/CONFIG name");
                                depth_ident_handled = 1;
                            }
                        }

                        if (!depth_ident_handled) {
                            /* Try to evaluate as a CONST expression (S7.10 allows
                             * CONST expressions in MEM depths).
                             */
                            long long dval = 0;
                            if (sem_eval_const_expr_in_module(mem->text, scope,
                                                              project_symbols, &dval) != 0) {
                                if (!depth_declared_ident) {
                                    sem_report_rule(diagnostics,
                                                    mem->loc,
                                                    "MEM_UNDEFINED_CONST_IN_WIDTH",
                                                    "MEM word width/depth uses undefined CONST name");
                                }
                            } else if (dval <= 0) {
                                sem_report_rule(diagnostics,
                                                mem->loc,
                                                "MEM_INVALID_DEPTH",
                                                "MEM depth must be a positive integer");
                            }
                        }
                    }
                }

                /* Validate initialization form: literal/constant expression vs
                 * @file("...") payload, plus file-size checks when widths and
                 * depths are simple positive integers.
                 */
                JZASTNode *init_expr = NULL;
                if (mem->child_count > 0) {
                    JZASTNode *first = mem->children[0];
                    if (first && first->type != JZ_AST_MEM_PORT) {
                        init_expr = first;
                    }
                }

                if (!init_expr) {
                    sem_report_rule(diagnostics,
                                    mem->loc,
                                    "MEM_MISSING_INIT",
                                    "MEM declared without required initialization literal or @file(...) payload");
                } else {
                    int have_word_w = (w_rc == 1 && word_w > 0);
                    int have_depth  = (d_rc == 1 && depth > 0);

                    if (init_expr->block_kind &&
                        strcmp(init_expr->block_kind, "FILE_REF") == 0 &&
                        init_expr->name) {
                        /* @file(CONST_REF) or @file(CONFIG.NAME) form:
                         * resolve the string reference and validate. */
                        const char *resolved = NULL;
                        if (sem_resolve_string_const(init_expr->name,
                                                     scope,
                                                     project_symbols,
                                                     &resolved,
                                                     diagnostics,
                                                     init_expr->loc)) {
                            jz_ast_set_text(init_expr, resolved);
                            sem_check_mem_file_init(mem,
                                                    init_expr,
                                                    word_w,
                                                    depth,
                                                    have_word_w,
                                                    have_depth,
                                                    diagnostics);
                        }
                        /* If resolution failed, diagnostic was already emitted. */
                    } else if (init_expr->type == JZ_AST_EXPR_LITERAL &&
                        init_expr->text &&
                        sem_mem_init_looks_like_file_path(init_expr->text)) {
                        /* @file("...") form: validate existence and size. */
                        sem_check_mem_file_init(mem,
                                                init_expr,
                                                word_w,
                                                depth,
                                                have_word_w,
                                                have_depth,
                                                diagnostics);
                    } else if (have_word_w) {
                        /* Literal or general expression initializer: infer
                         * bit-width and ensure it fits within the word width.
                         */
                        JZBitvecType init_ty;
                        init_ty.width = 0;
                        init_ty.is_signed = 0;
                        infer_expr_type(init_expr,
                                        scope,
                                        project_symbols,
                                        diagnostics,
                                        &init_ty);
                        if (init_ty.width > word_w) {
                            sem_report_rule(diagnostics,
                                            init_expr->loc,
                                            "MEM_INIT_LITERAL_OVERFLOW",
                                            "MEM initializer expression width exceeds declared word width");
                        }

                        /* MEM_INIT_CONTAINS_X: forbid x bits in literal-based
                         * initialization of MEM words. File-based initialization
                         * (handled above) is checked separately.
                         */
                        if (init_expr->type == JZ_AST_EXPR_LITERAL &&
                            init_expr->text &&
                            sem_literal_has_x_bits(init_expr->text)) {
                            sem_report_rule(diagnostics,
                                            init_expr->loc,
                                            "MEM_INIT_CONTAINS_X",
                                            "memory initialization literal must not contain x bits");
                        }

                        /* MEM_INIT_CONTAINS_Z: §1.6.7 forbids z bits in
                         * memory initialization values. Structural masking
                         * must eliminate all unknown bits before reaching
                         * MEM init sinks.
                         */
                        if (init_expr->type == JZ_AST_EXPR_LITERAL &&
                            init_expr->text &&
                            sem_literal_has_z_bits(init_expr->text)) {
                            sem_report_rule(diagnostics,
                                            init_expr->loc,
                                            "MEM_INIT_CONTAINS_Z",
                                            "memory initialization literal must not contain z bits");
                        }
                    }
                }

                /* Port list checks: at least one port, unique names within the
                 * MEM, and no conflicts with module-level identifiers.
                 */
                const size_t MAX_PORTS = 64;
                const char *names[MAX_PORTS];
                size_t name_count = 0;
                unsigned port_count = 0;

                for (size_t k = 0; k < mem->child_count; ++k) {
                    JZASTNode *port = mem->children[k];
                    if (!port || port->type != JZ_AST_MEM_PORT || !port->name) {
                        continue;
                    }
                    port_count++;

                    /* MEM_DUP_PORT_NAME within this MEM. */
                    int dup = 0;
                    for (size_t n = 0; n < name_count; ++n) {
                        if (names[n] && strcmp(names[n], port->name) == 0) {
                            sem_report_rule(diagnostics,
                                            port->loc,
                                            "MEM_DUP_PORT_NAME",
                                            "duplicate MEM port name inside MEM block");
                            dup = 1;
                            break;
                        }
                    }
                    if (!dup && name_count < MAX_PORTS) {
                        names[name_count++] = port->name;
                    }

                    /* MEM_PORT_NAME_CONFLICT_MODULE_ID: port name clashes with
                     * any module-level identifier (port/wire/register/CONST/etc.).
                     */
                    const JZSymbol *existing = module_scope_lookup(scope, port->name);
                    if (existing) {
                        sem_report_rule(diagnostics,
                                        port->loc,
                                        "MEM_PORT_NAME_CONFLICT_MODULE_ID",
                                        "MEM port name conflicts with module-level identifier");
                    }

                    /* MEM_TYPE_BLOCK_WITH_ASYNC_OUT: in TYPE=BLOCK memories,
                     * read ports (OUT) must be synchronous. Forbid ASYNC OUT.
                     */
                    if (type_is_block &&
                        port->block_kind && strcmp(port->block_kind, "OUT") == 0 &&
                        port->text && strcmp(port->text, "ASYNC") == 0) {
                        sem_report_rule(diagnostics,
                                        port->loc,
                                        "MEM_TYPE_BLOCK_WITH_ASYNC_OUT",
                                        "MEM(TYPE=BLOCK) cannot have OUT port declared ASYNC; OUT ports must be SYNC");
                    }

                    /* MEM_INVALID_WRITE_MODE: WRITE_MODE must be one of the
                     * supported values when present on a write (IN or INOUT) port.
                     */
                    if (port->block_kind &&
                        (strcmp(port->block_kind, "IN") == 0 ||
                         strcmp(port->block_kind, "INOUT") == 0) &&
                        port->text) {
                        const char *wm = port->text;
                        if (strcmp(wm, "WRITE_FIRST") != 0 &&
                            strcmp(wm, "READ_FIRST") != 0 &&
                            strcmp(wm, "NO_CHANGE") != 0) {
                            sem_report_rule(diagnostics,
                                            port->loc,
                                            "MEM_INVALID_WRITE_MODE",
                                            "MEM IN/INOUT port WRITE_MODE must be WRITE_FIRST, READ_FIRST, or NO_CHANGE");
                        }
                    }

                    /* MEM_INVALID_PORT_TYPE: reject invalid combinations of
                     * direction and qualifier on MEM ports.
                     */
                    const char *dir = port->block_kind;
                    const char *qual = port->text ? port->text : "";
                    if (!dir || (strcmp(dir, "IN") != 0 && strcmp(dir, "OUT") != 0 &&
                                 strcmp(dir, "INOUT") != 0)) {
                        sem_report_rule(diagnostics,
                                        port->loc,
                                        "MEM_INVALID_PORT_TYPE",
                                        "invalid MEM port direction; expected IN, OUT, or INOUT");
                    } else if (strcmp(dir, "IN") == 0) {
                        /* IN ports carry optional WRITE_MODE; qualifier is
                         * validated separately by MEM_INVALID_WRITE_MODE.
                         */
                    } else if (strcmp(dir, "OUT") == 0) {
                        /* OUT ports may be ASYNC or SYNC, or omit a qualifier
                         * (tool may default). Any other qualifier is invalid.
                         */
                        if (qual[0] &&
                            strcmp(qual, "ASYNC") != 0 &&
                            strcmp(qual, "SYNC") != 0) {
                            sem_report_rule(diagnostics,
                                            port->loc,
                                            "MEM_INVALID_PORT_TYPE",
                                            "invalid MEM OUT port qualifier; expected SYNC or ASYNC");
                        }
                    } else if (strcmp(dir, "INOUT") == 0) {
                        /* INOUT ports carry optional WRITE_MODE; qualifier is
                         * validated separately by MEM_INVALID_WRITE_MODE.
                         * ASYNC/SYNC keywords are already caught by parser.
                         */
                    }
                }

                /* MEM_INOUT_MIXED_WITH_IN_OUT: check for illegal mixing */
                int has_in_out = 0, has_inout = 0;
                for (size_t k2 = 0; k2 < mem->child_count; ++k2) {
                    JZASTNode *p2 = mem->children[k2];
                    if (!p2 || p2->type != JZ_AST_MEM_PORT || !p2->block_kind) continue;
                    if (strcmp(p2->block_kind, "INOUT") == 0) has_inout = 1;
                    else if (strcmp(p2->block_kind, "IN") == 0 ||
                             strcmp(p2->block_kind, "OUT") == 0) has_in_out = 1;
                }
                if (has_inout && has_in_out) {
                    sem_report_rule(diagnostics,
                                    mem->loc,
                                    "MEM_INOUT_MIXED_WITH_IN_OUT",
                                    "INOUT ports cannot be mixed with IN/OUT ports in the same MEM");
                }

                if (port_count == 0) {
                    sem_report_rule(diagnostics,
                                    mem->loc,
                                    "MEM_EMPTY_PORT_LIST",
                                    "MEM declared with no IN, OUT, or INOUT ports");
                }
            }
        } else if (child->type == JZ_AST_MUX_BLOCK) {
            /* Per-MUX validation for each declaration inside the MUX block. */
            for (size_t j = 0; j < child->child_count; ++j) {
                JZASTNode *mux = child->children[j];
                if (!mux || mux->type != JZ_AST_MUX_DECL) continue;

                if (mux->block_kind && strcmp(mux->block_kind, "AGGREGATE") == 0) {
                    sem_check_mux_aggregate_decl(mux, scope, diagnostics);
                } else if (mux->block_kind && strcmp(mux->block_kind, "SLICE") == 0) {
                    sem_check_mux_slice_decl(mux, scope, diagnostics);
                }
            }
        }
    }
}

/* -------------------------------------------------------------------------
 *  MUX declaration helpers used alongside MEM declarations
 * -------------------------------------------------------------------------
 */

/* Collapse whitespace from a substring and return a newly allocated identifier-
 * like string (used for MUX source lists). Returns NULL for empty results.
 */
static char *sem_normalize_name_segment(const char *start, size_t len)
{
    if (!start || len == 0) return NULL;
    size_t out_len = 0;
    for (size_t i = 0; i < len; ++i) {
        if (!isspace((unsigned char)start[i])) {
            out_len++;
        }
    }
    if (out_len == 0) return NULL;

    char *buf = (char *)malloc(out_len + 1);
    if (!buf) return NULL;
    size_t pos = 0;
    for (size_t i = 0; i < len; ++i) {
        if (!isspace((unsigned char)start[i])) {
            buf[pos++] = start[i];
        }
    }
    buf[pos] = '\0';
    return buf;
}

/* Resolve a simple MUX source name (bare identifier) to a module-level symbol
 * and, when possible, a concrete bit-width from the declaration's width
 * expression (only simple decimal widths are handled here).
 */
static int sem_mux_resolve_simple_source(const char *name,
                                         const JZModuleScope *scope,
                                         unsigned *out_width)
{
    if (!name || !scope) return 0;
    const JZSymbol *sym = module_scope_lookup(scope, name);
    if (!sym || !sym->node) return 0;

    /* Only ports, wires, and registers are considered valid aggregation
     * sources for now.
     */
    if (sym->kind != JZ_SYM_PORT &&
        sym->kind != JZ_SYM_WIRE &&
        sym->kind != JZ_SYM_REGISTER) {
        return 0;
    }

    if (out_width) {
        const char *wtext = sym->node->width;
        unsigned w = 0;
        int rc = eval_simple_positive_decl_int(wtext, &w);
        if (rc == 1) {
            *out_width = w;
        } else {
            *out_width = 0; /* unknown/complex width */
        }
    }
    return 1;
}

static void sem_check_mux_aggregate_decl(JZASTNode *mux_decl,
                                         const JZModuleScope *scope,
                                         JZDiagnosticList *diagnostics)
{
    if (!mux_decl || mux_decl->child_count == 0) return;
    JZASTNode *rhs = mux_decl->children[0];
    if (!rhs || rhs->type != JZ_AST_RAW_TEXT || !rhs->text) return;

    const char *text = rhs->text;
    const char *p = text;
    int reported_invalid = 0;
    int reported_width_mismatch = 0;
    unsigned ref_width = 0;
    int have_ref_width = 0;

    while (*p) {
        /* Find next comma-separated segment. */
        const char *seg_start = p;
        const char *comma = strchr(p, ',');
        size_t seg_len = 0;
        if (comma) {
            seg_len = (size_t)(comma - seg_start);
            p = comma + 1;
        } else {
            seg_len = strlen(seg_start);
            p = seg_start + seg_len;
        }

        char *name = sem_normalize_name_segment(seg_start, seg_len);
        if (!name) {
            continue; /* empty or all-whitespace segment */
        }

        unsigned w = 0;
        if (!sem_mux_resolve_simple_source(name, scope, &w)) {
            if (!reported_invalid) {
                sem_report_rule(diagnostics,
                                mux_decl->loc,
                                "MUX_AGG_SOURCE_INVALID",
                                "MUX aggregation source is not a valid readable signal in module scope");
                reported_invalid = 1;
            }
            free(name);
            continue;
        }

        if (w > 0) {
            if (!have_ref_width) {
                ref_width = w;
                have_ref_width = 1;
            } else if (w != ref_width && !reported_width_mismatch) {
                sem_report_rule(diagnostics,
                                mux_decl->loc,
                                "MUX_AGG_SOURCE_WIDTH_MISMATCH",
                                "MUX aggregation sources must all have identical bit-width");
                reported_width_mismatch = 1;
            }
        }

        free(name);
    }
}

static void sem_check_mux_slice_decl(JZASTNode *mux_decl,
                                     const JZModuleScope *scope,
                                     JZDiagnosticList *diagnostics)
{
    if (!mux_decl || mux_decl->child_count == 0) return;
    JZASTNode *rhs = mux_decl->children[0];
    if (!rhs || rhs->type != JZ_AST_RAW_TEXT || !rhs->text) return;

    /* Element width from mux_decl->width. */
    unsigned elem_w = 0;
    int rc = eval_simple_positive_decl_int(mux_decl->width, &elem_w);
    if (rc != 1 || elem_w == 0u) {
        return; /* unknown or invalid handled elsewhere */
    }

    /* Wide source is a single identifier in the RHS text. */
    char *wide_name = sem_normalize_name_segment(rhs->text, strlen(rhs->text));
    if (!wide_name) return;

    unsigned wide_w = 0;
    if (!sem_mux_resolve_simple_source(wide_name, scope, &wide_w) || wide_w == 0u) {
        free(wide_name);
        return;
    }
    free(wide_name);

    if (wide_w % elem_w != 0u) {
        sem_report_rule(diagnostics,
                        mux_decl->loc,
                        "MUX_SLICE_WIDTH_NOT_DIVISOR",
                        "MUX slice element_width must divide wide source width exactly");
    }
}

/* -------------------------------------------------------------------------
 *  MEM_WARN_PORT_NEVER_ACCESSED: warn when MEM IN/OUT ports are declared but
 *  never used in any mem.port[addr] access within the module.
 * -------------------------------------------------------------------------
 */
static int sem_mem_port_vec_contains(const JZBuffer *buf, JZASTNode *port)
{
    if (!buf || !port) return 0;
    size_t count = buf->len / sizeof(JZASTNode *);
    JZASTNode **arr = (JZASTNode **)buf->data;
    for (size_t i = 0; i < count; ++i) {
        if (arr[i] == port) return 1;
    }
    return 0;
}

static void sem_collect_mem_port_uses_recursive(JZASTNode *node,
                                                const JZModuleScope *scope,
                                                JZBuffer *used_ports)
{
    if (!node || !scope || !used_ports) return;

    if (node->type == JZ_AST_EXPR_SLICE && node->child_count >= 3) {
        JZMemPortRef ref;
        memset(&ref, 0, sizeof(ref));
        if (sem_match_mem_port_slice(node, scope, NULL, &ref) && ref.port) {
            if (!sem_mem_port_vec_contains(used_ports, ref.port)) {
                (void)jz_buf_append(used_ports, &ref.port, sizeof(ref.port));
            }
        }
    }
    if (node->type == JZ_AST_EXPR_QUALIFIED_IDENTIFIER) {
        JZMemPortRef ref;
        memset(&ref, 0, sizeof(ref));
        if (sem_match_mem_port_qualified_ident(node, scope, NULL, &ref) &&
            ref.port && (ref.field == MEM_PORT_FIELD_ADDR ||
                         ref.field == MEM_PORT_FIELD_DATA ||
                         ref.field == MEM_PORT_FIELD_WDATA)) {
            if (!sem_mem_port_vec_contains(used_ports, ref.port)) {
                (void)jz_buf_append(used_ports, &ref.port, sizeof(ref.port));
            }
        }
    }

    for (size_t i = 0; i < node->child_count; ++i) {
        sem_collect_mem_port_uses_recursive(node->children[i], scope, used_ports);
    }
}

static int sem_chip_mem_config_supported(const JZChipData *chip,
                                         JZChipMemType type,
                                         unsigned r_ports,
                                         unsigned w_ports,
                                         unsigned width,
                                         unsigned depth)
{
    if (!chip || chip->mem_configs.len == 0) return 1;
    const JZChipMemConfig *cfgs = (const JZChipMemConfig *)chip->mem_configs.data;
    size_t count = chip->mem_configs.len / sizeof(JZChipMemConfig);
    for (size_t i = 0; i < count; ++i) {
        if (cfgs[i].type != type) continue;
        if (cfgs[i].r_ports < r_ports) continue;
        if (cfgs[i].w_ports < w_ports) continue;
        if (type == JZ_CHIP_MEM_BLOCK) {
            /* BLOCK memories: synthesizer auto-splits oversized memories
             * into multiple BSRAM blocks.  Only port compatibility matters. */
            return 1;
        }
        /* DISTRIBUTED: must fit in a single config. */
        if (cfgs[i].width < width) continue;
        if (cfgs[i].depth < depth) continue;
        return 1;
    }
    return 0;
}

static void sem_mem_decl_port_counts(const JZASTNode *mem_decl,
                                     unsigned *out_r_ports,
                                     unsigned *out_w_ports,
                                     int *out_all_sync)
{
    unsigned r_ports = 0;
    unsigned w_ports = 0;
    int all_sync = 1;

    if (mem_decl) {
        for (size_t k = 0; k < mem_decl->child_count; ++k) {
            JZASTNode *port = mem_decl->children[k];
            if (!port || port->type != JZ_AST_MEM_PORT || !port->block_kind) {
                continue;
            }
            if (strcmp(port->block_kind, "OUT") == 0) {
                r_ports++;
                if (!port->text || strcmp(port->text, "SYNC") != 0) {
                    all_sync = 0;
                }
            } else if (strcmp(port->block_kind, "IN") == 0) {
                w_ports++;
            } else if (strcmp(port->block_kind, "INOUT") == 0) {
                /* INOUT counts as both read and write (always synchronous) */
                r_ports++;
                w_ports++;
                /* INOUT ports are always synchronous, so no change to all_sync */
            }
        }
    }

    if (out_r_ports) *out_r_ports = r_ports;
    if (out_w_ports) *out_w_ports = w_ports;
    if (out_all_sync) *out_all_sync = all_sync;
}

static JZChipMemType sem_mem_infer_type(unsigned depth, int have_depth, int all_sync)
{
    if (!have_depth) return JZ_CHIP_MEM_UNKNOWN;
    if (depth <= 16) {
        return JZ_CHIP_MEM_DISTRIBUTED;
    }
    if (all_sync) {
        return JZ_CHIP_MEM_BLOCK;
    }
    return JZ_CHIP_MEM_DISTRIBUTED;
}

/**
 * Compute how many BSRAM blocks a single BLOCK memory requires.
 *
 * blocks = ceil(width / cfg_w) * ceil(depth / cfg_d)
 *
 * Returns the minimum across all compatible configs, or 1 if no chip data.
 * Also returns the best config width/depth through out parameters (if non-NULL).
 */
static unsigned sem_compute_block_count(const JZChipData *chip,
                                        unsigned r_ports, unsigned w_ports,
                                        unsigned width, unsigned depth,
                                        unsigned *out_cfg_w, unsigned *out_cfg_d)
{
    if (!chip || chip->mem_configs.len == 0) {
        if (out_cfg_w) *out_cfg_w = 0;
        if (out_cfg_d) *out_cfg_d = 0;
        return 1;
    }
    const JZChipMemConfig *cfgs = (const JZChipMemConfig *)chip->mem_configs.data;
    size_t count = chip->mem_configs.len / sizeof(JZChipMemConfig);
    unsigned best = 0;
    unsigned best_w = 0, best_d = 0;
    for (size_t i = 0; i < count; ++i) {
        if (cfgs[i].type != JZ_CHIP_MEM_BLOCK) continue;
        if (cfgs[i].r_ports < r_ports) continue;
        if (cfgs[i].w_ports < w_ports) continue;
        unsigned cw = cfgs[i].width;
        unsigned cd = cfgs[i].depth;
        if (cw == 0 || cd == 0) continue;
        unsigned w_tiles = (width + cw - 1) / cw;
        unsigned d_tiles = (depth + cd - 1) / cd;
        unsigned total = w_tiles * d_tiles;
        if (best == 0 || total < best) {
            best = total;
            best_w = cw;
            best_d = cd;
        }
    }
    if (best == 0) {
        if (out_cfg_w) *out_cfg_w = 0;
        if (out_cfg_d) *out_cfg_d = 0;
        return 1;
    }
    if (out_cfg_w) *out_cfg_w = best_w;
    if (out_cfg_d) *out_cfg_d = best_d;
    return best;
}

void sem_check_module_mem_chip_configs(const JZModuleScope *scope,
                                       const JZBuffer *project_symbols,
                                       const JZChipData *chip,
                                       JZDiagnosticList *diagnostics)
{
    if (!scope || !scope->node || !chip || chip->mem_configs.len == 0) return;
    JZASTNode *mod = scope->node;
    if (mod->type == JZ_AST_BLACKBOX) return;

    for (size_t i = 0; i < mod->child_count; ++i) {
        JZASTNode *child = mod->children[i];
        if (!child || child->type != JZ_AST_MEM_BLOCK) continue;

        JZChipMemType header_type = sem_mem_header_parse_type(child->text);

        for (size_t j = 0; j < child->child_count; ++j) {
            JZASTNode *mem = child->children[j];
            if (!mem || mem->type != JZ_AST_MEM_DECL) continue;

            unsigned r_ports = 0;
            unsigned w_ports = 0;
            int all_sync = 1;
            sem_mem_decl_port_counts(mem, &r_ports, &w_ports, &all_sync);
            if (r_ports == 0 && w_ports == 0) {
                continue;
            }

            unsigned width = 0;
            unsigned depth = 0;
            long long depth_val = 0;
            int have_width = (sem_eval_width_expr_at_loc(mem->width,
                                                         scope,
                                                         project_symbols,
                                                         &width,
                                                         mem->loc) == 0 &&
                              width > 0);
            int have_depth = (sem_eval_const_expr_in_module(mem->text, scope, project_symbols, &depth_val) == 0 && depth_val > 0);
            if (have_depth) {
                depth = (unsigned)depth_val;
            }

            if (!have_width || !have_depth) {
                continue;
            }

            JZChipMemType mem_type = header_type;
            if (mem_type == JZ_CHIP_MEM_UNKNOWN) {
                mem_type = sem_mem_infer_type(depth, have_depth, all_sync);
            }
            if (mem_type == JZ_CHIP_MEM_UNKNOWN) {
                continue;
            }

            if (!sem_chip_mem_config_supported(chip, mem_type, r_ports, w_ports, width, depth)) {
                char msg[512];
                snprintf(msg, sizeof(msg),
                         "MEM [%u] [%u] with %u read, %u write port(s) does not match "
                         "any configuration for %s.\nSee --chip-info and --memory-report "
                         "for more information.",
                         width, depth, r_ports, w_ports,
                         chip->chip_id ? chip->chip_id : "selected chip");
                sem_report_rule(diagnostics,
                                mem->loc,
                                "MEM_CHIP_CONFIG_UNSUPPORTED",
                                msg);
            } else if (mem_type == JZ_CHIP_MEM_BLOCK) {
                unsigned cfg_w = 0, cfg_d = 0;
                unsigned blocks = sem_compute_block_count(chip, r_ports, w_ports,
                                                         width, depth, &cfg_w, &cfg_d);
                if (blocks > 1 && cfg_w > 0 && cfg_d > 0) {
                    unsigned w_tiles = (width + cfg_w - 1) / cfg_w;
                    unsigned d_tiles = (depth + cfg_d - 1) / cfg_d;
                    char msg[512];
                    snprintf(msg, sizeof(msg),
                             "MEM '%s' [%u] [%u] requires %u BSRAM blocks (%ux%u of [%u]x[%u])",
                             mem->name ? mem->name : "?",
                             width, depth, blocks, w_tiles, d_tiles, cfg_w, cfg_d);
                    sem_report_rule(diagnostics, mem->loc,
                                    "MEM_BLOCK_MULTI", msg);
                }
            }
        }
    }
}

/**
 * Check whether a latch directly drives a top-level output pin.
 *
 * A latch is "on a pin" when:
 *   1. The current module is the @top module.
 *   2. An ASYNC block contains `<port> <= <latch_name>` where the RHS is a
 *      bare identifier matching the latch name and the LHS is a PORT OUT.
 *   3. That port is bound in @top to a real OUT_PIN or INOUT_PIN (not `_`).
 */
static int latch_drives_output_pin(const JZModuleScope *scope,
                                   const char *latch_name,
                                   JZASTNode *project,
                                   const JZBuffer *project_symbols)
{
    if (!scope || !scope->node || !latch_name || !project || !project_symbols)
        return 0;

    JZASTNode *top_new = sem_find_project_top_new(project);
    if (!top_new || !top_new->name) return 0;

    /* The current module must be the @top module. */
    JZASTNode *mod = scope->node;
    if (!mod->name || strcmp(mod->name, top_new->name) != 0) return 0;

    /* Walk ASYNC blocks looking for `<port> <= <latch_name>`. */
    for (size_t i = 0; i < mod->child_count; ++i) {
        JZASTNode *blk = mod->children[i];
        if (!blk || blk->type != JZ_AST_BLOCK || !blk->block_kind) continue;
        if (strcmp(blk->block_kind, "ASYNCHRONOUS") != 0) continue;

        for (size_t j = 0; j < blk->child_count; ++j) {
            JZASTNode *stmt = blk->children[j];
            if (!stmt) continue;
            if (stmt->type != JZ_AST_STMT_ASSIGN) continue;
            if (stmt->child_count < 2) continue;
            /* Must be a <= (RECEIVE) assignment, not alias. */
            if (!stmt->block_kind || strcmp(stmt->block_kind, "RECEIVE") != 0)
                continue;

            /* RHS must be a bare identifier matching latch_name. */
            JZASTNode *rhs = stmt->children[1];
            if (!rhs || rhs->type != JZ_AST_EXPR_IDENTIFIER) continue;
            if (!rhs->name || strcmp(rhs->name, latch_name) != 0) continue;

            /* LHS must be a port name. */
            JZASTNode *lhs = stmt->children[0];
            if (!lhs || !lhs->name) continue;
            const char *port_name = lhs->name;

            /* Verify LHS is declared as PORT OUT in this module. */
            int is_port_out = 0;
            for (size_t pi = 0; pi < mod->child_count; ++pi) {
                JZASTNode *pb = mod->children[pi];
                if (!pb || pb->type != JZ_AST_PORT_BLOCK) continue;
                for (size_t pk = 0; pk < pb->child_count; ++pk) {
                    JZASTNode *pd = pb->children[pk];
                    if (!pd || pd->type != JZ_AST_PORT_DECL || !pd->name) continue;
                    if (strcmp(pd->name, port_name) != 0) continue;
                    if (pd->block_kind &&
                        (strcmp(pd->block_kind, "OUT") == 0 ||
                         strcmp(pd->block_kind, "INOUT") == 0)) {
                        is_port_out = 1;
                    }
                    break;
                }
                if (is_port_out) break;
            }
            if (!is_port_out) continue;

            /* Check that the port is bound in @top to a real pin. */
            for (size_t k = 0; k < top_new->child_count; ++k) {
                JZASTNode *b = top_new->children[k];
                if (!b || b->type != JZ_AST_PORT_DECL || !b->name) continue;
                if (strcmp(b->name, port_name) != 0) continue;

                /* Must be an OUT or INOUT binding. */
                if (!b->block_kind) break;
                if (strcmp(b->block_kind, "OUT") != 0 &&
                    strcmp(b->block_kind, "INOUT") != 0)
                    break;

                /* Must not be no-connect. */
                const char *target = b->text;
                if (!target || target[0] == '\0') break;
                if (target[0] == '_' && (target[1] == '\0' || target[1] == ' '))
                    break;

                /* Look up in project symbols as a real pin. */
                const JZSymbol *pin_sym = project_lookup(project_symbols,
                                                         target, JZ_SYM_PIN);
                if (pin_sym && pin_sym->node && pin_sym->node->block_kind &&
                    (strcmp(pin_sym->node->block_kind, "OUT_PINS") == 0 ||
                     strcmp(pin_sym->node->block_kind, "INOUT_PINS") == 0)) {
                    return 1;
                }
                break;
            }
        }
    }
    return 0;
}

void sem_check_module_latch_chip_support(const JZModuleScope *scope,
                                         const JZChipData *chip,
                                         JZASTNode *project,
                                         const JZBuffer *project_symbols,
                                         JZDiagnosticList *diagnostics)
{
    if (!scope || !scope->node || !chip || !chip->has_latches) return;
    JZASTNode *mod = scope->node;

    for (size_t i = 0; i < mod->child_count; ++i) {
        JZASTNode *child = mod->children[i];
        if (!child || child->type != JZ_AST_LATCH_BLOCK) continue;

        for (size_t j = 0; j < child->child_count; ++j) {
            JZASTNode *decl = child->children[j];
            if (!decl || decl->type != JZ_AST_LATCH_DECL) continue;

            const char *ltype = decl->block_kind;
            if (!ltype) continue;

            int on_pin = latch_drives_output_pin(scope, decl->name,
                                                  project, project_symbols);

            int supported = 0;
            if (on_pin) {
                /* Try IOB first, then fall back to fabric. */
                if (strcmp(ltype, "D") == 0) {
                    supported = chip->latches.iob_d || chip->latches.fab_d;
                } else if (strcmp(ltype, "SR") == 0) {
                    supported = chip->latches.iob_sr || chip->latches.fab_sr;
                }
            } else {
                /* Internal latch — fabric only. */
                if (strcmp(ltype, "D") == 0) {
                    supported = chip->latches.fab_d;
                } else if (strcmp(ltype, "SR") == 0) {
                    supported = chip->latches.fab_sr;
                }
            }

            if (!supported) {
                char msg[512];
                if (on_pin) {
                    snprintf(msg, sizeof(msg),
                             "%s latch not supported on this chip; "
                             "neither IOB nor CFU supports this latch type",
                             ltype);
                } else {
                    snprintf(msg, sizeof(msg),
                             "%s latch not supported in CFU on this chip; "
                             "CFU supports edge-triggered registers only",
                             ltype);
                }
                sem_report_rule(diagnostics, decl->loc,
                                "LATCH_CHIP_UNSUPPORTED", msg);
            }
        }
    }
}

void sem_check_module_mem_port_usage(const JZModuleScope *scope,
                                     JZDiagnosticList *diagnostics)
{
    if (!scope || !scope->node) return;
    JZASTNode *mod = scope->node;
    if (mod->type == JZ_AST_BLACKBOX) return;

    /* Collect all MEM ports declared in this module. */
    JZBuffer all_ports = (JZBuffer){0};
    for (size_t i = 0; i < mod->child_count; ++i) {
        JZASTNode *child = mod->children[i];
        if (!child || child->type != JZ_AST_MEM_BLOCK) continue;
        for (size_t j = 0; j < child->child_count; ++j) {
            JZASTNode *mem = child->children[j];
            if (!mem || mem->type != JZ_AST_MEM_DECL) continue;
            for (size_t k = 0; k < mem->child_count; ++k) {
                JZASTNode *port = mem->children[k];
                if (!port || port->type != JZ_AST_MEM_PORT || !port->name) continue;
                (void)jz_buf_append(&all_ports, &port, sizeof(port));
            }
        }
    }

    if (all_ports.len == 0) {
        jz_buf_free(&all_ports);
        return;
    }

    /* Traverse the module AST to find all mem.port[addr] usages. */
    JZBuffer used_ports = (JZBuffer){0};
    sem_collect_mem_port_uses_recursive(mod, scope, &used_ports);

    /* Emit MEM_WARN_PORT_NEVER_ACCESSED for ports that are never used. */
    JZASTNode **ports = (JZASTNode **)all_ports.data;
    size_t port_count = all_ports.len / sizeof(JZASTNode *);
    for (size_t i = 0; i < port_count; ++i) {
        JZASTNode *port = ports[i];
        if (!port) continue;
        if (sem_mem_port_vec_contains(&used_ports, port)) {
            continue;
        }
        /* Only warn for IN/OUT/INOUT style ports; block_kind may be NULL for
         * malformed ASTs, which we skip conservatively.
         */
        if (!port->block_kind) continue;
        if (strcmp(port->block_kind, "IN") != 0 &&
            strcmp(port->block_kind, "OUT") != 0 &&
            strcmp(port->block_kind, "INOUT") != 0) {
            continue;
        }
        sem_report_rule(diagnostics,
                        port->loc,
                        "MEM_WARN_PORT_NEVER_ACCESSED",
                        "MEM IN/OUT/INOUT port declared but never used");
    }

    jz_buf_free(&all_ports);
    jz_buf_free(&used_ports);
}

/* -------------------------------------------------------------------------
 *  Project-wide memory resource usage check
 * -------------------------------------------------------------------------
 */

/** @brief Accumulates the number of times each module is instantiated. */
typedef struct JZModuleInstanceCount {
    const char *module_name; /**< Module identifier text. */
    unsigned    count;       /**< Total number of instances in the project graph. */
} JZModuleInstanceCount;

typedef struct JZModuleVisitFrame {
    const char *module_name;
} JZModuleVisitFrame;

/* Recursively accumulate instance counts starting from a given module.
 * Each entry in `counts` stores the total number of times a module is
 * instantiated across the design hierarchy.  `multiplier` is the number
 * of times the current module itself is instantiated.
 */
static void mem_res_count_instances(JZASTNode *project,
                                     const char *module_name,
                                     unsigned multiplier,
                                     const JZBuffer *module_scopes,
                                     const JZBuffer *project_symbols,
                                     JZBuffer *counts,
                                     JZBuffer *active_path)
{
    if (!module_name || !project || !counts) return;

    if (active_path) {
        size_t active_count = active_path->len / sizeof(JZModuleVisitFrame);
        JZModuleVisitFrame *frames = (JZModuleVisitFrame *)active_path->data;
        for (size_t i = 0; i < active_count; ++i) {
            if (frames[i].module_name &&
                strcmp(frames[i].module_name, module_name) == 0) {
                return;
            }
        }

        {
            JZModuleVisitFrame frame;
            frame.module_name = module_name;
            if (jz_buf_append(active_path, &frame, sizeof(frame)) != 0) {
                return;
            }
        }
    }

    /* Find the module AST node. */
    JZASTNode *mod = NULL;
    for (size_t i = 0; i < project->child_count; ++i) {
        JZASTNode *child = project->children[i];
        if (child && child->type == JZ_AST_MODULE && child->name &&
            strcmp(child->name, module_name) == 0) {
            mod = child;
            break;
        }
    }
    if (!mod) {
        if (active_path && active_path->len >= sizeof(JZModuleVisitFrame)) {
            active_path->len -= sizeof(JZModuleVisitFrame);
        }
        return;
    }

    /* Add/update the count for this module. */
    size_t n = counts->len / sizeof(JZModuleInstanceCount);
    JZModuleInstanceCount *arr = (JZModuleInstanceCount *)counts->data;
    int found = 0;
    for (size_t i = 0; i < n; ++i) {
        if (strcmp(arr[i].module_name, module_name) == 0) {
            arr[i].count += multiplier;
            found = 1;
            break;
        }
    }
    if (!found) {
        JZModuleInstanceCount mc;
        mc.module_name = module_name;
        mc.count = multiplier;
        jz_buf_append(counts, &mc, sizeof(mc));
    }

    /* Find the module scope for CONST evaluation. */
    const JZModuleScope *scope = NULL;
    if (module_scopes) {
        size_t sc = module_scopes->len / sizeof(JZModuleScope);
        const JZModuleScope *scopes = (const JZModuleScope *)module_scopes->data;
        for (size_t i = 0; i < sc; ++i) {
            if (scopes[i].node == mod) {
                scope = &scopes[i];
                break;
            }
        }
    }

    /* Walk child instances (including inside FEATURE_GUARD). */
    for (size_t i = 0; i < mod->child_count; ++i) {
        JZASTNode *child = mod->children[i];
        if (!child) continue;

        if (child->type == JZ_AST_MODULE_INSTANCE && child->text) {
            unsigned array_count = 1;
            if (child->width && *child->width) {
                long long val = 0;
                if (scope && sem_eval_const_expr_in_module(child->width, scope,
                        project_symbols, &val) == 0 && val > 0) {
                    array_count = (unsigned)val;
                } else {
                    unsigned v = 0;
                    if (eval_simple_positive_decl_int(child->width, &v) == 1 && v > 0)
                        array_count = v;
                }
            }
            mem_res_count_instances(project, child->text,
                                     multiplier * array_count,
                                     module_scopes, project_symbols,
                                     counts, active_path);
        } else if (child->type == JZ_AST_FEATURE_GUARD) {
            /* Walk branches of FEATURE_GUARD for instances. */
            for (size_t bi = 1; bi < child->child_count; ++bi) {
                JZASTNode *branch = child->children[bi];
                if (!branch) continue;
                for (size_t bj = 0; bj < branch->child_count; ++bj) {
                    JZASTNode *bchild = branch->children[bj];
                    if (bchild && bchild->type == JZ_AST_MODULE_INSTANCE && bchild->text) {
                        unsigned array_count = 1;
                        if (bchild->width && *bchild->width) {
                            long long val = 0;
                            if (scope && sem_eval_const_expr_in_module(bchild->width, scope,
                                    project_symbols, &val) == 0 && val > 0) {
                                array_count = (unsigned)val;
                            } else {
                                unsigned v = 0;
                                if (eval_simple_positive_decl_int(bchild->width, &v) == 1 && v > 0)
                                    array_count = v;
                            }
                        }
                        mem_res_count_instances(project, bchild->text,
                                                 multiplier * array_count,
                                                 module_scopes, project_symbols,
                                                 counts, active_path);
                    }
                }
            }
        }
    }

    if (active_path && active_path->len >= sizeof(JZModuleVisitFrame)) {
        active_path->len -= sizeof(JZModuleVisitFrame);
    }
}

void sem_check_project_mem_resources(JZASTNode *project,
                                     const JZBuffer *module_scopes,
                                     const JZBuffer *project_symbols,
                                     const JZChipData *chip,
                                     JZDiagnosticList *diagnostics)
{
    if (!project || project->type != JZ_AST_PROJECT || !chip || !diagnostics) return;

    unsigned block_limit = jz_chip_mem_quantity(chip, JZ_CHIP_MEM_BLOCK);
    unsigned dist_limit_bits = jz_chip_mem_total_bits(chip, JZ_CHIP_MEM_DISTRIBUTED);
    if (block_limit == 0 && dist_limit_bits == 0) return;

    JZASTNode *top_new = sem_find_project_top_new(project);
    if (!top_new || !top_new->name) return;

    /* Build instance count map from @top downward. */
    JZBuffer counts = {0};
    JZBuffer active_path = {0};
    mem_res_count_instances(project, top_new->name, 1,
                             module_scopes, project_symbols,
                             &counts, &active_path);
    jz_buf_free(&active_path);

    /* Accumulate total BLOCK count and DISTRIBUTED bits. */
    unsigned total_block_count = 0;
    unsigned long long total_dist_bits = 0;

    size_t mc_count = counts.len / sizeof(JZModuleInstanceCount);
    const JZModuleInstanceCount *mc_arr = (const JZModuleInstanceCount *)counts.data;

    for (size_t mi = 0; mi < mc_count; ++mi) {
        const char *mod_name = mc_arr[mi].module_name;
        unsigned inst_count = mc_arr[mi].count;

        /* Find the module AST node. */
        JZASTNode *mod = NULL;
        for (size_t i = 0; i < project->child_count; ++i) {
            JZASTNode *child = project->children[i];
            if (child && child->type == JZ_AST_MODULE && child->name &&
                strcmp(child->name, mod_name) == 0) {
                mod = child;
                break;
            }
        }
        if (!mod) continue;

        /* Find the module scope. */
        const JZModuleScope *scope = NULL;
        if (module_scopes) {
            size_t sc = module_scopes->len / sizeof(JZModuleScope);
            const JZModuleScope *scopes = (const JZModuleScope *)module_scopes->data;
            for (size_t i = 0; i < sc; ++i) {
                if (scopes[i].node == mod) {
                    scope = &scopes[i];
                    break;
                }
            }
        }

        /* Walk MEM blocks in this module. */
        for (size_t i = 0; i < mod->child_count; ++i) {
            JZASTNode *child = mod->children[i];
            if (!child || child->type != JZ_AST_MEM_BLOCK) continue;

            JZChipMemType header_type = sem_mem_header_parse_type(child->text);

            for (size_t j = 0; j < child->child_count; ++j) {
                JZASTNode *mem = child->children[j];
                if (!mem || mem->type != JZ_AST_MEM_DECL) continue;

                unsigned width = 0;
                unsigned depth = 0;
                long long depth_val = 0;
                int have_width = (scope && sem_eval_width_expr_at_loc(mem->width,
                                   scope, project_symbols, &width, mem->loc) == 0 && width > 0);
                int have_depth = (scope && sem_eval_const_expr_in_module(mem->text, scope,
                                   project_symbols, &depth_val) == 0 && depth_val > 0);
                if (have_depth) depth = (unsigned)depth_val;

                JZChipMemType mem_type = header_type;
                if (mem_type == JZ_CHIP_MEM_UNKNOWN && have_depth) {
                    unsigned r_ports = 0, w_ports = 0;
                    int all_sync = 1;
                    sem_mem_decl_port_counts(mem, &r_ports, &w_ports, &all_sync);
                    mem_type = sem_mem_infer_type(depth, have_depth, all_sync);
                }

                if (mem_type == JZ_CHIP_MEM_BLOCK) {
                    if (have_width && have_depth) {
                        unsigned r_p = 0, w_p = 0;
                        int as = 1;
                        sem_mem_decl_port_counts(mem, &r_p, &w_p, &as);
                        unsigned blocks = sem_compute_block_count(chip, r_p, w_p,
                                                                 width, depth,
                                                                 NULL, NULL);
                        total_block_count += blocks * inst_count;
                    } else {
                        total_block_count += inst_count;
                    }
                } else if (mem_type == JZ_CHIP_MEM_DISTRIBUTED && have_width && have_depth) {
                    total_dist_bits += (unsigned long long)width * depth * inst_count;
                }
            }
        }
    }

    /* Report if limits exceeded. */
    if (block_limit > 0 && total_block_count > block_limit) {
        char msg[512];
        snprintf(msg, sizeof(msg),
                 "design uses %u BLOCK memory instances but chip '%s' has only %u BSRAM blocks",
                 total_block_count,
                 chip->chip_id ? chip->chip_id : "selected chip",
                 block_limit);
        sem_report_rule(diagnostics, top_new->loc,
                        "MEM_BLOCK_RESOURCE_EXCEEDED", msg);
    }

    if (dist_limit_bits > 0 && total_dist_bits > dist_limit_bits) {
        char msg[512];
        snprintf(msg, sizeof(msg),
                 "design uses %llu bits of DISTRIBUTED memory but chip '%s' has only %u bits",
                 (unsigned long long)total_dist_bits,
                 chip->chip_id ? chip->chip_id : "selected chip",
                 dist_limit_bits);
        sem_report_rule(diagnostics, top_new->loc,
                        "MEM_DISTRIBUTED_RESOURCE_EXCEEDED", msg);
    }

    jz_buf_free(&counts);
}
