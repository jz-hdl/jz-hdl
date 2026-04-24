#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ir_builder.h"
#include "ir.h"
#include "diagnostic.h"
#include "util.h"

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

static void report_init_lowering_error(JZDiagnosticList *diagnostics,
                                       const char *message)
{
    if (!diagnostics || !message) return;
    JZLocation loc = {0};
    jz_diagnostic_report(diagnostics, loc, JZ_SEVERITY_ERROR,
                         "MEM_INIT_LOWERING", message);
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
        char msg[1024];
        snprintf(msg, sizeof(msg),
                 "failed to read memory initialization file: %s",
                 file_path ? file_path : "<null>");
        report_init_lowering_error(diagnostics, msg);
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
            char msg[1024];
            snprintf(msg, sizeof(msg),
                     "memory initialization file contains x or z values: %s",
                     file_path ? file_path : "<null>");
            report_init_lowering_error(diagnostics, msg);
            free(contents);
            return -1;
        }

        if (is_hex) {
            unsigned value;
            if (ch >= '0' && ch <= '9') {
                value = (unsigned)(ch - '0');
            } else if (ch >= 'a' && ch <= 'f') {
                value = 10u + (unsigned)(ch - 'a');
            } else if (ch >= 'A' && ch <= 'F') {
                value = 10u + (unsigned)(ch - 'A');
            } else {
                continue;
            }
            if (append_bits_from_value(blob_bytes,
                                       bytes_per_word,
                                       word_width,
                                       capacity_bits,
                                       &bit_index,
                                       value,
                                       4) != 0) {
                char msg[1024];
                snprintf(msg, sizeof(msg),
                         "memory initialization file exceeds declared memory size: %s",
                         file_path ? file_path : "<null>");
                report_init_lowering_error(diagnostics, msg);
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
                char msg[1024];
                snprintf(msg, sizeof(msg),
                         "memory initialization file exceeds declared memory size: %s",
                         file_path ? file_path : "<null>");
                report_init_lowering_error(diagnostics, msg);
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
        char msg[1024];
        snprintf(msg, sizeof(msg),
                 "failed to read memory initialization file: %s",
                 file_path ? file_path : "<null>");
        report_init_lowering_error(diagnostics, msg);
        return -1;
    }

    if (file_size > capacity_bytes) {
        char msg[1024];
        snprintf(msg, sizeof(msg),
                 "memory initialization file exceeds declared memory size: %s",
                 file_path ? file_path : "<null>");
        report_init_lowering_error(diagnostics, msg);
        free(contents);
        return -1;
    }

    memcpy(blob_bytes, contents, file_size);
    free(contents);
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
            const char *ext = mem_init_get_ext(file_path);
            int is_hex = ext && mem_init_ext_eq(ext, "hex");
            int is_mem = ext && mem_init_ext_eq(ext, "mem");
            int is_bin = ext && mem_init_ext_eq(ext, "bin");

            if (!is_hex && !is_mem && !is_bin) {
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

            int bytes_per_word = (mem->word_width + 7) / 8;
            size_t capacity_bytes = (size_t)bytes_per_word * (size_t)mem->depth;
            size_t capacity_bits = (size_t)mem->word_width * (size_t)mem->depth;

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

            if (is_hex || is_mem) {
                if (lower_text_mem_init(file_path,
                                        is_hex,
                                        blob->bytes,
                                        bytes_per_word,
                                        mem->word_width,
                                        capacity_bits,
                                        diagnostics) != 0) {
                    return -1;
                }
            } else {
                if (lower_binary_mem_init(file_path,
                                          blob->bytes,
                                          capacity_bytes,
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
