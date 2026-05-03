/**
 * @file util.h
 * @brief General-purpose utilities for string handling and dynamic buffers.
 *
 * Provides functions for string duplication, file I/O, and a growable
 * byte buffer implementation.
 */

#ifndef JZ_HDL_UTIL_H
#define JZ_HDL_UTIL_H

#include <stddef.h>
#include <stdio.h>

#define JZ_MAX_SOURCE_FILE_BYTES            (16u * 1024u * 1024u)
#define JZ_MAX_SOURCE_TOKENS                (1024u * 1024u)
#define JZ_MAX_LOCAL_CHIP_JSON_BYTES        (4u * 1024u * 1024u)
#define JZ_MAX_LOCAL_CHIP_JSON_TOKENS       (256u * 1024u)
#define JZ_MAX_CHIP_JSON_NESTING_DEPTH      128u
#define JZ_MAX_MEM_INIT_FILE_BYTES          (64u * 1024u * 1024u)
#define JZ_MAX_MEM_INIT_MIF_DEPTH           (1024u * 1024u)
#define JZ_MAX_PARSER_EXPR_DEPTH            512u
#define JZ_MAX_PARSER_STATEMENT_DEPTH       512u
#define JZ_MAX_CONST_EVAL_DEPTH             512u
#define JZ_MAX_IR_EXPR_DEPTH                512u
#define JZ_MAX_IR_STMT_DEPTH                512u
#define JZ_MAX_AST_DEPTH                    4096u
#define JZ_MAX_TEMPLATE_EXPAND_DEPTH        1024u
#define JZ_MAX_TRISTATE_DEPTH               1024u
#define JZ_MAX_SEM_RECURSION_DEPTH          1024u
#define JZ_MAX_MAP_PIN_WIDTH                (1024u * 1024u)
#define JZ_MAX_MAP_BITMAP_BYTES             (8u * 1024u * 1024u)
#define JZ_MAX_SIM_MEMORY_DEPTH             (1024u * 1024u)

/**
 * @brief Duplicate a string using malloc.
 * @param s String to duplicate.
 * @return Newly allocated copy, or NULL on failure.
 * @note Caller must free the result with free().
 */
char *jz_strdup(const char *s);

/**
 * @brief Read an entire file into memory.
 * @param filename Path to the file.
 * @param out_size Optional pointer to receive file size.
 * @return File contents as null-terminated string, or NULL on failure.
 * @note Caller must free the result with free().
 */
char *jz_read_entire_file(const char *filename, size_t *out_size);

/**
 * @brief Read an entire file into memory after checking a hard byte limit.
 * @param filename Path to the file.
 * @param max_size Maximum allowed file size in bytes.
 * @param out_size Optional pointer to receive file size.
 * @return File contents as null-terminated string, or NULL on failure/limit hit.
 */
char *jz_read_entire_file_limit(const char *filename, size_t max_size, size_t *out_size);

/**
 * @brief Read an entire open FILE* into memory after checking a hard byte limit.
 * @param fp Open file positioned anywhere.
 * @param max_size Maximum allowed file size in bytes.
 * @param out_size Optional pointer to receive file size.
 * @return File contents as null-terminated string, or NULL on failure/limit hit.
 */
char *jz_read_entire_fp_limit(FILE *fp, size_t max_size, size_t *out_size);

/**
 * @brief Determine file size without reading the full file contents.
 * @param filename Path to the file.
 * @param out_size Output size on success.
 * @return 0 on success, -1 on failure.
 */
int jz_get_file_size(const char *filename, size_t *out_size);

/**
 * @brief Determine the size of an already-open file without consuming it.
 * @param fp Open file handle.
 * @param out_size Output size on success.
 * @return 0 on success, -1 on failure.
 */
int jz_get_fp_size(FILE *fp, size_t *out_size);

/**
 * @brief Add two size_t values with overflow checking.
 * @return 0 on success, -1 on overflow or invalid output pointer.
 */
int jz_size_add_checked(size_t a, size_t b, size_t *out);

/**
 * @brief Multiply two size_t values with overflow checking.
 * @return 0 on success, -1 on overflow or invalid output pointer.
 */
int jz_size_mul_checked(size_t a, size_t b, size_t *out);

/**
 * @brief Round a size up to the next multiple of alignment.
 * @return 0 on success, -1 on overflow, invalid output pointer, or zero/non-power-of-two alignment.
 */
int jz_size_align_up_checked(size_t size, size_t alignment, size_t *out);

/**
 * @struct JZBuffer
 * @brief A growable dynamic byte buffer.
 */
typedef struct JZBuffer {
    unsigned char *data;  /* Pointer to buffer data. */
    size_t         len;   /* Current length in bytes. */
    size_t         cap;   /* Allocated capacity in bytes. */
} JZBuffer;

/**
 * @brief Reserve capacity in a buffer.
 * @param buf Pointer to the buffer.
 * @param new_cap Minimum capacity required.
 * @return 0 on success, -1 on allocation failure.
 */
int   jz_buf_reserve(JZBuffer *buf, size_t new_cap);

/**
 * @brief Append data to a buffer.
 * @param buf Pointer to the buffer.
 * @param data Pointer to data to append.
 * @param len Number of bytes to append.
 * @return 0 on success, -1 on allocation failure.
 */
int   jz_buf_append(JZBuffer *buf, const void *data, size_t len);

/**
 * @brief Free a buffer and reset its state.
 * @param buf Pointer to the buffer.
 */
void  jz_buf_free(JZBuffer *buf);

/**
 * @brief Open a unique temporary output file next to a target path using exclusive creation.
 * @param target Final output path.
 * @param out Receives writable stream on success.
 * @param tmp_path Receives actual temporary path on success.
 * @param tmp_path_size Size of tmp_path buffer.
 * @return 0 on success, -1 on failure.
 */
int jz_open_exclusive_temp_output(const char *target,
                                  FILE **out,
                                  char *tmp_path,
                                  size_t tmp_path_size);

/**
 * @brief Flush, close, and atomically rename a temporary output into place.
 * @param out Open stream returned by jz_open_exclusive_temp_output().
 * @param tmp_path Temporary path.
 * @param final_path Final destination path.
 * @return 0 on success, -1 on failure.
 */
int jz_commit_exclusive_temp_output(FILE *out,
                                    const char *tmp_path,
                                    const char *final_path);

/**
 * @brief Create a unique sidecar file path using exclusive creation.
 * @param prefix Sidecar filename prefix.
 * @param suffix Sidecar filename suffix, including dot if needed.
 * @param out Receives writable stream on success.
 * @param path_buf Receives the chosen sidecar path on success.
 * @param path_buf_size Size of path_buf buffer.
 * @return 0 on success, -1 on failure.
 */
int jz_open_unique_sidecar_output(const char *prefix,
                                  const char *suffix,
                                  FILE **out,
                                  char *path_buf,
                                  size_t path_buf_size);

#endif /* JZ_HDL_UTIL_H */
