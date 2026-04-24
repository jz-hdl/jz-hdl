#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "ir_builder.h"
#include "ir.h"
#include "diagnostic.h"
#include "util.h"

typedef enum {
    MEM_FILE_FORMAT_UNKNOWN = 0,
    MEM_FILE_FORMAT_BIN,
    MEM_FILE_FORMAT_HEX,
    MEM_FILE_FORMAT_MEM,
    MEM_FILE_FORMAT_MIF,
    MEM_FILE_FORMAT_COE
} MemInitFileFormat;

typedef enum {
    MEM_RADIX_NONE = 0,
    MEM_RADIX_BIN = 2,
    MEM_RADIX_OCT = 8,
    MEM_RADIX_DEC = 10,
    MEM_RADIX_HEX = 16,
    MEM_RADIX_UNS = 100
} MemInitRadix;

#define MEM_INIT_BLOB_MAX_BYTES ((size_t)256u * 1024u * 1024u)

static const char *mem_init_get_ext(const char *path)
{
    if (!path) return NULL;
    const char *slash = strrchr(path, '/');
    const char *name = slash ? slash + 1 : path;
    const char *dot = strrchr(name, '.');
    if (!dot || !dot[1]) return NULL;
    return dot + 1;
}

static int mem_init_ext_eq(const char *ext, const char *want)
{
    if (!ext || !want) return 0;
    while (*ext && *want) {
        int c1 = tolower((unsigned char)*ext);
        int c2 = tolower((unsigned char)*want);
        if (c1 != c2) return 0;
        ++ext;
        ++want;
    }
    return *ext == '\0' && *want == '\0';
}

static MemInitFileFormat mem_init_get_format(const char *file_path)
{
    const char *ext = mem_init_get_ext(file_path);
    if (ext && mem_init_ext_eq(ext, "hex")) return MEM_FILE_FORMAT_HEX;
    if (ext && mem_init_ext_eq(ext, "mem")) return MEM_FILE_FORMAT_MEM;
    if (ext && mem_init_ext_eq(ext, "bin")) return MEM_FILE_FORMAT_BIN;
    if (ext && mem_init_ext_eq(ext, "mif")) return MEM_FILE_FORMAT_MIF;
    if (ext && mem_init_ext_eq(ext, "coe")) return MEM_FILE_FORMAT_COE;
    return MEM_FILE_FORMAT_UNKNOWN;
}

static void report_init_lowering_error(JZDiagnosticList *diagnostics,
                                       const char *message)
{
    if (!diagnostics || !message) return;
    JZLocation loc = {0};
    jz_diagnostic_report(diagnostics, loc, JZ_SEVERITY_ERROR,
                         "MEM_INIT_LOWERING", message);
}

static void report_file_error(JZDiagnosticList *diagnostics,
                              const char *file_path,
                              const char *detail)
{
    char msg[1024];
    snprintf(msg, sizeof(msg), "%s: %s",
             detail ? detail : "memory initialization error",
             file_path ? file_path : "<null>");
    report_init_lowering_error(diagnostics, msg);
}

static void set_blob_bit(uint8_t *blob_bytes,
                         int bytes_per_word,
                         int word_width,
                         size_t bit_index)
{
    size_t word_index = bit_index / (size_t)word_width;
    int bit_in_word = (int)(bit_index % (size_t)word_width);
    int target_bit = word_width - 1 - bit_in_word;
    int byte_in_word = bytes_per_word - 1 - (target_bit / 8);
    int bit_in_byte = target_bit % 8;
    size_t byte_index = word_index * (size_t)bytes_per_word + (size_t)byte_in_word;
    blob_bytes[byte_index] |= (uint8_t)(1u << bit_in_byte);
}

static void clear_blob_word(uint8_t *blob_bytes,
                            int bytes_per_word,
                            unsigned long long addr)
{
    memset(blob_bytes + ((size_t)addr * (size_t)bytes_per_word), 0,
           (size_t)bytes_per_word);
}

static void set_blob_word_lsb_bit(uint8_t *blob_bytes,
                                  int bytes_per_word,
                                  unsigned long long addr,
                                  int bit_from_lsb)
{
    size_t word_base = (size_t)addr * (size_t)bytes_per_word;
    int byte_in_word = bytes_per_word - 1 - (bit_from_lsb / 8);
    int bit_in_byte = bit_from_lsb % 8;
    blob_bytes[word_base + (size_t)byte_in_word] |= (uint8_t)(1u << bit_in_byte);
}

static int append_bits_from_value(uint8_t *blob_bytes,
                                  int bytes_per_word,
                                  int word_width,
                                  size_t capacity_bits,
                                  size_t *bit_index,
                                  unsigned value,
                                  int bit_count)
{
    for (int bit = bit_count - 1; bit >= 0; --bit) {
        if (*bit_index >= capacity_bits) {
            return -1;
        }
        if ((value >> bit) & 1u) {
            set_blob_bit(blob_bytes, bytes_per_word, word_width, *bit_index);
        }
        (*bit_index)++;
    }
    return -1;
}

static int mem_init_ci_char_eq(char a, char b)
{
    return tolower((unsigned char)a) == tolower((unsigned char)b);
}

static int mem_init_ci_prefix_eq(const char *text, const char *prefix)
{
    while (*text && *prefix) {
        if (!mem_init_ci_char_eq(*text, *prefix)) return 0;
        ++text;
        ++prefix;
    }
    return *prefix == '\0';
}

static int mem_init_is_ident_char(char ch)
{
    return isalnum((unsigned char)ch) || ch == '_';
}

static const char *mem_init_find_keyword_ci(const char *text, const char *keyword)
{
    size_t key_len;
    if (!text || !keyword) return NULL;
    key_len = strlen(keyword);
    if (key_len == 0) return NULL;

    for (const char *p = text; *p; ++p) {
        if ((p == text || !mem_init_is_ident_char(p[-1])) &&
            mem_init_ci_prefix_eq(p, keyword) &&
            !mem_init_is_ident_char(p[key_len])) {
            return p;
        }
    }
    return NULL;
}

static void mem_init_trim_span(const char **start, const char **end)
{
    while (*start < *end && isspace((unsigned char)**start)) {
        (*start)++;
    }
    while (*end > *start && isspace((unsigned char)(*end)[-1])) {
        (*end)--;
    }
}

static char *mem_init_dup_span(const char *start, const char *end)
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

static int mem_init_extract_assignment(const char *text,
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
        mem_init_trim_span(&lhs_start, &lhs_end);
        if (lhs_start < lhs_end) {
            eq = memchr(lhs_start, '=', (size_t)(lhs_end - lhs_start));
            if (eq) {
                lhs_end = eq;
                rhs_start = eq + 1;
                rhs_end = stmt_end;
                mem_init_trim_span(&lhs_start, &lhs_end);
                mem_init_trim_span(&rhs_start, &rhs_end);
                if ((size_t)(lhs_end - lhs_start) == key_len &&
                    mem_init_ci_prefix_eq(lhs_start, keyword)) {
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

static char *mem_init_strip_mif_comments(const char *contents, size_t file_size)
{
    char *out = (char *)malloc(file_size + 1);
    size_t in = 0;
    size_t out_len = 0;
    if (!out) return NULL;

    while (in < file_size) {
        if (contents[in] == '%' ) {
            ++in;
            while (in < file_size && contents[in] != '%') {
                ++in;
            }
            if (in < file_size) ++in;
            continue;
        }
        if (contents[in] == '-' && in + 1 < file_size && contents[in + 1] == '-') {
            in += 2;
            while (in < file_size &&
                   contents[in] != '\n' &&
                   contents[in] != '\r') {
                ++in;
            }
            continue;
        }
        out[out_len++] = contents[in++];
    }

    out[out_len] = '\0';
    return out;
}

static int mem_init_hex_digit_value(char ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return 10 + (ch - 'a');
    if (ch >= 'A' && ch <= 'F') return 10 + (ch - 'A');
    return -1;
}

static int mem_init_octal_digit_value(char ch)
{
    if (ch >= '0' && ch <= '7') return ch - '0';
    return -1;
}

static int mem_init_binary_digit_value(char ch)
{
    if (ch == '0' || ch == '1') return ch - '0';
    return -1;
}

static int mem_init_parse_radix_string(const char *start,
                                       const char *end,
                                       MemInitRadix *out_radix)
{
    char *copy;
    MemInitRadix radix = MEM_RADIX_NONE;
    if (!start || !end || !out_radix) return -1;
    copy = mem_init_dup_span(start, end);
    if (!copy) return -1;

    if (mem_init_ci_prefix_eq(copy, "BIN") && strlen(copy) == 3) {
        radix = MEM_RADIX_BIN;
    } else if (mem_init_ci_prefix_eq(copy, "OCT") && strlen(copy) == 3) {
        radix = MEM_RADIX_OCT;
    } else if (mem_init_ci_prefix_eq(copy, "DEC") && strlen(copy) == 3) {
        radix = MEM_RADIX_DEC;
    } else if (mem_init_ci_prefix_eq(copy, "HEX") && strlen(copy) == 3) {
        radix = MEM_RADIX_HEX;
    } else if (mem_init_ci_prefix_eq(copy, "UNS") && strlen(copy) == 3) {
        radix = MEM_RADIX_UNS;
    }

    free(copy);
    if (radix == MEM_RADIX_NONE) return -1;
    *out_radix = radix;
    return 0;
}

static int mem_init_parse_coe_radix(const char *start,
                                    const char *end,
                                    MemInitRadix *out_radix)
{
    char *copy;
    if (!start || !end || !out_radix) return -1;
    copy = mem_init_dup_span(start, end);
    if (!copy) return -1;

    if (strcmp(copy, "2") == 0) {
        *out_radix = MEM_RADIX_BIN;
    } else if (strcmp(copy, "10") == 0) {
        *out_radix = MEM_RADIX_DEC;
    } else if (strcmp(copy, "16") == 0) {
        *out_radix = MEM_RADIX_HEX;
    } else {
        free(copy);
        return -1;
    }

    free(copy);
    return 0;
}

static int mem_init_token_has_xz(const char *start, const char *end)
{
    for (const char *p = start; p < end; ++p) {
        if (*p == 'x' || *p == 'X' || *p == 'z' || *p == 'Z') {
            return 1;
        }
    }
    return 0;
}

static int mem_init_parse_unsigned_value(const char *start,
                                         const char *end,
                                         MemInitRadix radix,
                                         unsigned long long *out_value)
{
    char *copy;
    char *clean;
    char *dst;
    char *endptr;
    unsigned long long value;
    int base;

    if (!start || !end || !out_value) return -1;
    copy = mem_init_dup_span(start, end);
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

    if (clean[0] == '\0' || clean[0] == '-') {
        free(clean);
        free(copy);
        return -1;
    }

    if (radix == MEM_RADIX_BIN) base = 2;
    else if (radix == MEM_RADIX_OCT) base = 8;
    else if (radix == MEM_RADIX_HEX) base = 16;
    else if (radix == MEM_RADIX_DEC || radix == MEM_RADIX_UNS) base = 10;
    else {
        free(clean);
        free(copy);
        return -1;
    }

    value = strtoull(clean, &endptr, base);
    if (!endptr || *endptr != '\0') {
        free(clean);
        free(copy);
        return -1;
    }
    free(clean);
    free(copy);
    *out_value = value;
    return 0;
}

static int mem_init_store_int_value(uint8_t *blob_bytes,
                                    int bytes_per_word,
                                    int word_width,
                                    unsigned long long addr,
                                    unsigned long long value)
{
    clear_blob_word(blob_bytes, bytes_per_word, addr);
    for (int bit = 0; bit < word_width && bit < 64; ++bit) {
        if ((value >> bit) & 1ull) {
            set_blob_word_lsb_bit(blob_bytes, bytes_per_word, addr, bit);
        }
    }
    return 0;
}

static int mem_init_store_decimal_token(uint8_t *blob_bytes,
                                        int bytes_per_word,
                                        int word_width,
                                        unsigned long long addr,
                                        const char *token_start,
                                        const char *token_end,
                                        int signed_decimal,
                                        const char *file_path,
                                        JZDiagnosticList *diagnostics)
{
    char *copy;
    char *clean;
    char *dst;
    char *endptr;
    int negative = 0;

    if (mem_init_token_has_xz(token_start, token_end)) {
        report_file_error(diagnostics, file_path,
                          "memory initialization file contains x or z values");
        return -1;
    }

    copy = mem_init_dup_span(token_start, token_end);
    if (!copy) {
        report_file_error(diagnostics, file_path,
                          "failed to allocate decimal memory init token");
        return -1;
    }

    clean = (char *)malloc(strlen(copy) + 1);
    if (!clean) {
        free(copy);
        report_file_error(diagnostics, file_path,
                          "failed to allocate decimal memory init token");
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

    if (clean[0] == '\0') {
        free(clean);
        report_file_error(diagnostics, file_path,
                          "invalid decimal value in memory initialization file");
        return -1;
    }

    if (signed_decimal && clean[0] == '-') {
        unsigned long long encoded;
        long long sval = strtoll(clean, &endptr, 10);
        if (!endptr || *endptr != '\0') {
            free(clean);
            report_file_error(diagnostics, file_path,
                              "invalid decimal value in memory initialization file");
            return -1;
        }
        negative = 1;
        encoded = (unsigned long long)sval;
        clear_blob_word(blob_bytes, bytes_per_word, addr);
        for (int bit = 0; bit < word_width; ++bit) {
            int set = 0;
            if (bit < 64) {
                set = ((encoded >> bit) & 1ull) != 0;
            } else if (negative) {
                set = 1;
            }
            if (set) {
                set_blob_word_lsb_bit(blob_bytes, bytes_per_word, addr, bit);
            }
        }
        free(clean);
        return 0;
    }

    if (!signed_decimal && clean[0] == '-') {
        free(clean);
        report_file_error(diagnostics, file_path,
                          "negative value not allowed for unsigned memory initialization radix");
        return -1;
    }

    {
        unsigned long long uval = strtoull(clean, &endptr, 10);
        if (!endptr || *endptr != '\0') {
            free(clean);
            report_file_error(diagnostics, file_path,
                              "invalid decimal value in memory initialization file");
            return -1;
        }
        free(clean);
        return mem_init_store_int_value(blob_bytes, bytes_per_word,
                                        word_width, addr, uval);
    }
}

static int mem_init_store_radix_token(uint8_t *blob_bytes,
                                      int bytes_per_word,
                                      int word_width,
                                      unsigned long long addr,
                                      const char *token_start,
                                      const char *token_end,
                                      MemInitRadix radix,
                                      const char *file_path,
                                      JZDiagnosticList *diagnostics)
{
    int bits_per_digit;
    int bit_pos = 0;

    if (radix == MEM_RADIX_DEC || radix == MEM_RADIX_UNS) {
        return mem_init_store_decimal_token(blob_bytes, bytes_per_word,
                                            word_width, addr,
                                            token_start, token_end,
                                            radix == MEM_RADIX_DEC,
                                            file_path, diagnostics);
    }

    if (radix == MEM_RADIX_BIN) bits_per_digit = 1;
    else if (radix == MEM_RADIX_OCT) bits_per_digit = 3;
    else if (radix == MEM_RADIX_HEX) bits_per_digit = 4;
    else {
        report_file_error(diagnostics, file_path,
                          "unsupported radix in memory initialization file");
        return -1;
    }

    clear_blob_word(blob_bytes, bytes_per_word, addr);
    for (const char *p = token_end; p > token_start;) {
        int digit = -1;
        char ch;
        --p;
        ch = *p;
        if (ch == '_') continue;

        if (ch == 'x' || ch == 'X' || ch == 'z' || ch == 'Z') {
            report_file_error(diagnostics, file_path,
                              "memory initialization file contains x or z values");
            return -1;
        }

        if (radix == MEM_RADIX_BIN) digit = mem_init_binary_digit_value(ch);
        else if (radix == MEM_RADIX_OCT) digit = mem_init_octal_digit_value(ch);
        else if (radix == MEM_RADIX_HEX) digit = mem_init_hex_digit_value(ch);

        if (digit < 0) {
            report_file_error(diagnostics, file_path,
                              "invalid digit in memory initialization file");
            return -1;
        }

        for (int bit = 0; bit < bits_per_digit; ++bit) {
            if ((digit >> bit) & 1) {
                if (bit_pos >= word_width) {
                    report_file_error(diagnostics, file_path,
                                      "memory initialization word exceeds declared width");
                    return -1;
                }
                set_blob_word_lsb_bit(blob_bytes, bytes_per_word, addr, bit_pos);
            }
            ++bit_pos;
        }
    }

    return 0;
}

static int lower_text_mem_init(const char *file_path,
                               int is_hex,
                               uint8_t *blob_bytes,
                               int bytes_per_word,
                               int word_width,
                               size_t capacity_bits,
                               JZDiagnosticList *diagnostics)
{
    size_t file_size = 0;
    char *contents = jz_read_entire_file(file_path, &file_size);
    if (!contents) {
        report_file_error(diagnostics, file_path,
                          "failed to read memory initialization file");
        return -1;
    }

    size_t bit_index = 0;
    for (size_t i = 0; i < file_size; ++i) {
        unsigned char ch = (unsigned char)contents[i];

        if (ch == '/' && i + 1 < file_size && contents[i + 1] == '/') {
            i += 2;
            while (i < file_size && contents[i] != '\n' && contents[i] != '\r') {
                ++i;
            }
            if (i >= file_size) break;
            continue;
        }

        if (isspace(ch) || ch == '_') {
            continue;
        }

        if (ch == 'x' || ch == 'X' || ch == 'z' || ch == 'Z') {
            report_file_error(diagnostics, file_path,
                              "memory initialization file contains x or z values");
            free(contents);
            return -1;
        }

        if (is_hex) {
            unsigned value;
            int digit = mem_init_hex_digit_value((char)ch);
            if (digit < 0) {
                continue;
            }
            value = (unsigned)digit;
            if (append_bits_from_value(blob_bytes,
                                       bytes_per_word,
                                       word_width,
                                       capacity_bits,
                                       &bit_index,
                                       value,
                                       4) != 0) {
                report_file_error(diagnostics, file_path,
                                  "memory initialization file exceeds declared memory size");
                free(contents);
                return -1;
            }
        } else {
            if (ch != '0' && ch != '1') {
                continue;
            }
            if (append_bits_from_value(blob_bytes,
                                       bytes_per_word,
                                       word_width,
                                       capacity_bits,
                                       &bit_index,
                                       (unsigned)(ch - '0'),
                                       1) != 0) {
                report_file_error(diagnostics, file_path,
                                  "memory initialization file exceeds declared memory size");
                free(contents);
                return -1;
            }
        }
    }

    free(contents);
    return 0;
}

static int lower_binary_mem_init(const char *file_path,
                                 uint8_t *blob_bytes,
                                 size_t capacity_bytes,
                                 JZDiagnosticList *diagnostics)
{
    size_t file_size = 0;
    char *contents = jz_read_entire_file(file_path, &file_size);
    if (!contents) {
        report_file_error(diagnostics, file_path,
                          "failed to read memory initialization file");
        return -1;
    }

    if (file_size > capacity_bytes) {
        report_file_error(diagnostics, file_path,
                          "memory initialization file exceeds declared memory size");
        free(contents);
        return -1;
    }

    memcpy(blob_bytes, contents, file_size);
    free(contents);
    return 0;
}

static int lower_coe_mem_init(const char *file_path,
                              uint8_t *blob_bytes,
                              int bytes_per_word,
                              int word_width,
                              int depth,
                              JZDiagnosticList *diagnostics)
{
    size_t file_size = 0;
    char *contents = jz_read_entire_file(file_path, &file_size);
    const char *radix_start;
    const char *radix_end;
    const char *vec_start;
    const char *vec_end;
    MemInitRadix radix;
    int addr = 0;

    if (!contents) {
        report_file_error(diagnostics, file_path,
                          "failed to read memory initialization file");
        return -1;
    }

    if (mem_init_extract_assignment(contents,
                                    "memory_initialization_radix",
                                    &radix_start, &radix_end) != 0 ||
        mem_init_parse_coe_radix(radix_start, radix_end, &radix) != 0) {
        free(contents);
        report_file_error(diagnostics, file_path,
                          "unsupported or missing MEMORY_INITIALIZATION_RADIX in COE file");
        return -1;
    }

    if (mem_init_extract_assignment(contents,
                                    "memory_initialization_vector",
                                    &vec_start, &vec_end) != 0) {
        free(contents);
        report_file_error(diagnostics, file_path,
                          "missing MEMORY_INITIALIZATION_VECTOR in COE file");
        return -1;
    }

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
        if (tok_start == tok_end) {
            continue;
        }
        if (addr >= depth) {
            free(contents);
            report_file_error(diagnostics, file_path,
                              "memory initialization file exceeds declared memory size");
            return -1;
        }
        if (mem_init_store_radix_token(blob_bytes, bytes_per_word, word_width,
                                       (unsigned long long)addr,
                                       tok_start, tok_end, radix,
                                       file_path, diagnostics) != 0) {
            free(contents);
            return -1;
        }
        ++addr;
    }

    free(contents);
    return 0;
}

static int lower_mif_mem_init(const char *file_path,
                              uint8_t *blob_bytes,
                              int bytes_per_word,
                              int word_width,
                              int depth,
                              JZDiagnosticList *diagnostics)
{
    size_t file_size = 0;
    char *contents = jz_read_entire_file(file_path, &file_size);
    char *stripped = NULL;
    const char *depth_start;
    const char *depth_end;
    const char *width_start;
    const char *width_end;
    const char *addr_radix_start;
    const char *addr_radix_end;
    const char *data_radix_start;
    const char *data_radix_end;
    const char *content_kw;
    const char *begin_kw;
    const char *end_kw;
    unsigned long long mif_depth = 0;
    unsigned long long mif_width = 0;
    MemInitRadix addr_radix = MEM_RADIX_NONE;
    MemInitRadix data_radix = MEM_RADIX_NONE;

    if (!contents) {
        report_file_error(diagnostics, file_path,
                          "failed to read memory initialization file");
        return -1;
    }

    stripped = mem_init_strip_mif_comments(contents, file_size);
    free(contents);
    if (!stripped) {
        report_file_error(diagnostics, file_path,
                          "failed to process MIF file");
        return -1;
    }

    if (mem_init_extract_assignment(stripped, "DEPTH",
                                    &depth_start, &depth_end) != 0 ||
        mem_init_parse_unsigned_value(depth_start, depth_end, MEM_RADIX_DEC,
                                      &mif_depth) != 0) {
        free(stripped);
        report_file_error(diagnostics, file_path,
                          "missing or invalid DEPTH in MIF file");
        return -1;
    }

    if (mem_init_extract_assignment(stripped, "WIDTH",
                                    &width_start, &width_end) != 0 ||
        mem_init_parse_unsigned_value(width_start, width_end, MEM_RADIX_DEC,
                                      &mif_width) != 0) {
        free(stripped);
        report_file_error(diagnostics, file_path,
                          "missing or invalid WIDTH in MIF file");
        return -1;
    }

    if ((int)mif_width != word_width || (int)mif_depth != depth) {
        free(stripped);
        report_file_error(diagnostics, file_path,
                          "MIF DEPTH/WIDTH do not match memory declaration");
        return -1;
    }

    if (mem_init_extract_assignment(stripped, "ADDRESS_RADIX",
                                    &addr_radix_start, &addr_radix_end) != 0 ||
        mem_init_parse_radix_string(addr_radix_start, addr_radix_end,
                                    &addr_radix) != 0) {
        free(stripped);
        report_file_error(diagnostics, file_path,
                          "missing or invalid ADDRESS_RADIX in MIF file");
        return -1;
    }

    if (mem_init_extract_assignment(stripped, "DATA_RADIX",
                                    &data_radix_start, &data_radix_end) != 0 ||
        mem_init_parse_radix_string(data_radix_start, data_radix_end,
                                    &data_radix) != 0) {
        free(stripped);
        report_file_error(diagnostics, file_path,
                          "missing or invalid DATA_RADIX in MIF file");
        return -1;
    }

    content_kw = mem_init_find_keyword_ci(stripped, "CONTENT");
    begin_kw = content_kw ? mem_init_find_keyword_ci(content_kw, "BEGIN") : NULL;
    end_kw = begin_kw ? mem_init_find_keyword_ci(begin_kw, "END") : NULL;
    if (!content_kw || !begin_kw || !end_kw || end_kw <= begin_kw) {
        free(stripped);
        report_file_error(diagnostics, file_path,
                          "missing CONTENT BEGIN ... END block in MIF file");
        return -1;
    }

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

            if (!stmt_end || stmt_end > end_kw) {
                break;
            }
            entry_start = cursor;
            entry_end = stmt_end;
            mem_init_trim_span(&entry_start, &entry_end);
            cursor = stmt_end + 1;
            if (entry_start >= entry_end) {
                continue;
            }

            colon = memchr(entry_start, ':', (size_t)(entry_end - entry_start));
            if (!colon) {
                continue;
            }

            addr_spec_start = entry_start;
            addr_spec_end = colon;
            data_spec_start = colon + 1;
            data_spec_end = entry_end;
            mem_init_trim_span(&addr_spec_start, &addr_spec_end);
            mem_init_trim_span(&data_spec_start, &data_spec_end);

            if (addr_spec_start < addr_spec_end && *addr_spec_start == '[') {
                const char *inner_start = addr_spec_start + 1;
                const char *inner_end = addr_spec_end;
                const char *dots;
                if (addr_spec_end <= addr_spec_start + 2 ||
                    addr_spec_end[-1] != ']') {
                    free(stripped);
                    report_file_error(diagnostics, file_path,
                                      "invalid address range in MIF file");
                    return -1;
                }
                inner_end--;
                dots = NULL;
                for (const char *p = inner_start; p + 1 < inner_end; ++p) {
                    if (p[0] == '.' && p[1] == '.') {
                        dots = p;
                        break;
                    }
                }
                if (!dots) {
                    free(stripped);
                    report_file_error(diagnostics, file_path,
                                      "invalid address range in MIF file");
                    return -1;
                }
                {
                    const char *lhs_start = inner_start;
                    const char *lhs_end = dots;
                    const char *rhs_start = dots + 2;
                    const char *rhs_end = inner_end;
                    mem_init_trim_span(&lhs_start, &lhs_end);
                    mem_init_trim_span(&rhs_start, &rhs_end);
                    if (mem_init_parse_unsigned_value(lhs_start, lhs_end,
                                                      addr_radix, &start_addr) != 0 ||
                        mem_init_parse_unsigned_value(rhs_start, rhs_end,
                                                      addr_radix, &end_addr) != 0 ||
                        end_addr < start_addr) {
                        free(stripped);
                        report_file_error(diagnostics, file_path,
                                          "invalid address range in MIF file");
                        return -1;
                    }
                }
                is_range = 1;
            } else {
                if (mem_init_parse_unsigned_value(addr_spec_start, addr_spec_end,
                                                  addr_radix, &start_addr) != 0) {
                    free(stripped);
                    report_file_error(diagnostics, file_path,
                                      "invalid address in MIF file");
                    return -1;
                }
                end_addr = start_addr;
            }

            {
                const char *p = data_spec_start;
                const char *tok_start = NULL;
                const char *tok_end = NULL;
                const char *first_token_start = NULL;
                const char *first_token_end = NULL;
                int token_count = 0;
                unsigned long long current_addr = start_addr;

                while (p <= data_spec_end) {
                    int at_end = (p == data_spec_end);
                    if (!at_end && *p != ',' && !isspace((unsigned char)*p)) {
                        if (!tok_start) tok_start = p;
                        ++p;
                        continue;
                    }
                    if (tok_start) {
                        tok_end = p;
                        if (!first_token_start) {
                            first_token_start = tok_start;
                            first_token_end = tok_end;
                        }
                        ++token_count;
                        if (!is_range || token_count <= (int)(end_addr - start_addr + 1ull)) {
                            if (current_addr >= (unsigned long long)depth) {
                                free(stripped);
                                report_file_error(diagnostics, file_path,
                                                  "memory initialization file exceeds declared memory size");
                                return -1;
                            }
                            if (mem_init_store_radix_token(blob_bytes, bytes_per_word,
                                                           word_width, current_addr,
                                                           tok_start, tok_end,
                                                           data_radix,
                                                           file_path,
                                                           diagnostics) != 0) {
                                free(stripped);
                                return -1;
                            }
                            ++current_addr;
                        }
                        tok_start = NULL;
                    }
                    ++p;
                }

                if (token_count == 0) {
                    free(stripped);
                    report_file_error(diagnostics, file_path,
                                      "missing data value in MIF file");
                    return -1;
                }

                if (is_range && token_count == 1) {
                    for (unsigned long long addr = start_addr + 1; addr <= end_addr; ++addr) {
                        if (addr >= (unsigned long long)depth) {
                            free(stripped);
                            report_file_error(diagnostics, file_path,
                                              "memory initialization file exceeds declared memory size");
                            return -1;
                        }
                        if (mem_init_store_radix_token(blob_bytes, bytes_per_word,
                                                       word_width, addr,
                                                       first_token_start, first_token_end,
                                                       data_radix, file_path,
                                                       diagnostics) != 0) {
                            free(stripped);
                            return -1;
                        }
                    }
                } else if (is_range && start_addr <= end_addr) {
                    const char *q = data_spec_start;
                    const char *seq_start = NULL;
                    const char *seq_end = NULL;
                    int seq_index = 0;
                    for (unsigned long long addr = start_addr; addr <= end_addr; ++addr) {
                        if (addr >= (unsigned long long)depth) {
                            free(stripped);
                            report_file_error(diagnostics, file_path,
                                              "memory initialization file exceeds declared memory size");
                            return -1;
                        }
                        q = data_spec_start;
                        seq_start = NULL;
                        seq_end = NULL;
                        for (int want = 0; want <= (seq_index % token_count); ++want) {
                            while (q <= data_spec_end &&
                                   (q == data_spec_end ||
                                    *q == ',' ||
                                    isspace((unsigned char)*q))) {
                                ++q;
                            }
                            seq_start = q;
                            while (q < data_spec_end &&
                                   *q != ',' &&
                                   !isspace((unsigned char)*q)) {
                                ++q;
                            }
                            seq_end = q;
                        }
                        if (!seq_start || seq_start >= seq_end) {
                            free(stripped);
                            report_file_error(diagnostics, file_path,
                                              "invalid data sequence in MIF file");
                            return -1;
                        }
                        if (mem_init_store_radix_token(blob_bytes, bytes_per_word,
                                                       word_width, addr,
                                                       seq_start, seq_end,
                                                       data_radix,
                                                       file_path,
                                                       diagnostics) != 0) {
                            free(stripped);
                            return -1;
                        }
                        ++seq_index;
                    }
                }

                if (!is_range && token_count > 1) {
                    unsigned long long addr = start_addr;
                    const char *q = data_spec_start;
                    while (q <= data_spec_end) {
                        const char *seq_start = NULL;
                        const char *seq_end = NULL;
                        while (q <= data_spec_end &&
                               (q == data_spec_end ||
                                *q == ',' ||
                                isspace((unsigned char)*q))) {
                            ++q;
                        }
                        seq_start = q;
                        while (q < data_spec_end &&
                               *q != ',' &&
                               !isspace((unsigned char)*q)) {
                            ++q;
                        }
                        seq_end = q;
                        if (!seq_start || seq_start >= seq_end) {
                            continue;
                        }
                        if (addr >= (unsigned long long)depth) {
                            free(stripped);
                            report_file_error(diagnostics, file_path,
                                              "memory initialization file exceeds declared memory size");
                            return -1;
                        }
                        if (mem_init_store_radix_token(blob_bytes, bytes_per_word,
                                                       word_width, addr,
                                                       seq_start, seq_end,
                                                       data_radix,
                                                       file_path,
                                                       diagnostics) != 0) {
                            free(stripped);
                            return -1;
                        }
                        ++addr;
                    }
                }
            }
        }
    }

    free(stripped);
    return 0;
}

int jz_ir_init_lowering(IR_Design *design,
                        JZArena *arena,
                        JZDiagnosticList *diagnostics)
{
    if (!design || !arena) return -1;

    for (int mi = 0; mi < design->num_modules; ++mi) {
        IR_Module *mod = &design->modules[mi];
        for (int memi = 0; memi < mod->num_memories; ++memi) {
            IR_Memory *mem = &mod->memories[memi];
            if (mem->init_kind != MEM_INIT_FILE || !mem->init.file_path) {
                continue;
            }

            const char *file_path = mem->init.file_path;
            MemInitFileFormat format = mem_init_get_format(file_path);

            if (format == MEM_FILE_FORMAT_UNKNOWN) {
                char msg[1024];
                snprintf(msg, sizeof(msg),
                         "unsupported backend memory initialization format: %s",
                         file_path);
                report_init_lowering_error(diagnostics, msg);
                return -1;
            }

            if (mem->word_width <= 0 || mem->depth <= 0) {
                char msg[1024];
                snprintf(msg, sizeof(msg),
                         "cannot lower memory initialization without concrete width/depth: %s.%s",
                         mod->name ? mod->name : "jz_module",
                         mem->name ? mem->name : "jz_mem");
                report_init_lowering_error(diagnostics, msg);
                return -1;
            }

            size_t bytes_per_word_size = ((size_t)mem->word_width + 7u) / 8u;
            size_t capacity_bytes = 0;
            size_t capacity_bits = 0;
            int bytes_per_word;

            if ((size_t)mem->depth > SIZE_MAX / bytes_per_word_size ||
                (size_t)mem->depth > SIZE_MAX / (size_t)mem->word_width) {
                char msg[1024];
                snprintf(msg, sizeof(msg),
                         "memory init blob too large to lower safely: %s.%s",
                         mod->name ? mod->name : "jz_module",
                         mem->name ? mem->name : "jz_mem");
                report_init_lowering_error(diagnostics, msg);
                return -1;
            }

            capacity_bytes = bytes_per_word_size * (size_t)mem->depth;
            capacity_bits = (size_t)mem->word_width * (size_t)mem->depth;
            if (capacity_bytes > MEM_INIT_BLOB_MAX_BYTES ||
                capacity_bytes > (size_t)INT_MAX) {
                char msg[1024];
                snprintf(msg, sizeof(msg),
                         "memory init blob too large (%zu bytes, max %zu): %s.%s",
                         capacity_bytes, MEM_INIT_BLOB_MAX_BYTES,
                         mod->name ? mod->name : "jz_module",
                         mem->name ? mem->name : "jz_mem");
                report_init_lowering_error(diagnostics, msg);
                return -1;
            }
            bytes_per_word = (int)bytes_per_word_size;

            IR_MemoryInitBlob *blob = (IR_MemoryInitBlob *)jz_arena_alloc(
                arena, sizeof(IR_MemoryInitBlob));
            if (!blob) {
                report_init_lowering_error(diagnostics,
                                           "failed to allocate lowered memory init blob");
                return -1;
            }
            blob->num_bytes = (int)capacity_bytes;
            blob->bytes = NULL;

            if (capacity_bytes > 0) {
                blob->bytes = (uint8_t *)jz_arena_alloc(arena, capacity_bytes);
                if (!blob->bytes) {
                    report_init_lowering_error(diagnostics,
                                               "failed to allocate lowered memory init bytes");
                    return -1;
                }
                memset(blob->bytes, 0, capacity_bytes);
            }

            if (format == MEM_FILE_FORMAT_HEX || format == MEM_FILE_FORMAT_MEM) {
                if (lower_text_mem_init(file_path,
                                        format == MEM_FILE_FORMAT_HEX,
                                        blob->bytes,
                                        bytes_per_word,
                                        mem->word_width,
                                        capacity_bits,
                                        diagnostics) != 0) {
                    return -1;
                }
            } else if (format == MEM_FILE_FORMAT_BIN) {
                if (lower_binary_mem_init(file_path,
                                          blob->bytes,
                                          capacity_bytes,
                                          diagnostics) != 0) {
                    return -1;
                }
            } else if (format == MEM_FILE_FORMAT_COE) {
                if (lower_coe_mem_init(file_path,
                                       blob->bytes,
                                       bytes_per_word,
                                       mem->word_width,
                                       mem->depth,
                                       diagnostics) != 0) {
                    return -1;
                }
            } else if (format == MEM_FILE_FORMAT_MIF) {
                if (lower_mif_mem_init(file_path,
                                       blob->bytes,
                                       bytes_per_word,
                                       mem->word_width,
                                       mem->depth,
                                       diagnostics) != 0) {
                    return -1;
                }
            }

            mem->init.blob = blob;
            mem->init_kind = MEM_INIT_BLOB;
        }
    }

    return 0;
}
