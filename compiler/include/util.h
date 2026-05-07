/**
 * @file util.h
 * @brief General-purpose helpers for file I/O, bounds checks, and buffers.
 *
 * Provides string duplication, bounded file-reading helpers, centralized
 * compiler hard-limit lookups, checked size arithmetic, recursion guards,
 * and a growable byte buffer implementation.
 */

#ifndef JZ_HDL_UTIL_H
#define JZ_HDL_UTIL_H

#include <stddef.h>
#include <stdio.h>

#define JZ_MAX_SOURCE_FILE_BYTES            (16u * 1024u * 1024u)
#define JZ_MAX_SOURCE_TOKENS                (1024u * 1024u)
#define JZ_MAX_IMPORT_DEPTH                 64u
#define JZ_MAX_IMPORT_RETAINED_SOURCE_BYTES (16u * 1024u * 1024u)
#define JZ_MAX_IMPORT_RETAINED_TOKEN_BYTES  (64u * 1024u * 1024u)
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
#define JZ_MAX_REPORT_RECURSION_DEPTH       1024u
#define JZ_MAX_CHIP_VARIANT_TUPLES          (1024u * 1024u)
#define JZ_MAX_IR_EXPANDED_ITEMS            (1024u * 1024u)
#define JZ_MAX_SEM_BRANCH_STATES            (64u * 1024u)
#define JZ_MAX_MAP_PIN_WIDTH                (1024u * 1024u)
#define JZ_MAX_MAP_BITMAP_BYTES             (8u * 1024u * 1024u)
#define JZ_MAX_SIM_INSTANCE_DEPTH           1024u
#define JZ_MAX_SIM_MEMORY_DEPTH             (1024u * 1024u)
#define JZ_MAX_SIM_MEMORY_OBJECT_BYTES      (128u * 1024u * 1024u)
#define JZ_MAX_EMITTED_TRACE_BYTES          (64u * 1024u * 1024u)
#define JZ_MAX_RTLIL_MEM_INIT_EMIT_BYTES    (64u * 1024u * 1024u)

/**
 * @enum JZInputLimitKind
 * @brief Named compiler hard-limit categories returned by jz_input_limit_value().
 */
typedef enum JZInputLimitKind {
    JZ_LIMIT_SOURCE_FILE_BYTES = 0,        /**< Maximum source file size in bytes. */
    JZ_LIMIT_SOURCE_TOKENS,                /**< Maximum token count for a source file. */
    JZ_LIMIT_IMPORT_DEPTH,                 /**< Maximum nested `@import` depth. */
    JZ_LIMIT_IMPORT_RETAINED_SOURCE_BYTES, /**< Maximum retained imported source bytes. */
    JZ_LIMIT_IMPORT_RETAINED_TOKEN_BYTES,  /**< Maximum retained imported token bytes. */
    JZ_LIMIT_CHIP_JSON_BYTES,              /**< Maximum chip JSON file size in bytes. */
    JZ_LIMIT_CHIP_JSON_TOKENS,             /**< Maximum chip JSON token count. */
    JZ_LIMIT_CHIP_JSON_NESTING_DEPTH,      /**< Maximum chip JSON nesting depth. */
    JZ_LIMIT_MEM_INIT_FILE_BYTES,          /**< Maximum memory init file size in bytes. */
    JZ_LIMIT_MEM_INIT_MIF_DEPTH,           /**< Maximum parsed MIF nesting depth. */
    JZ_LIMIT_PARSER_EXPR_DEPTH,            /**< Maximum parser expression recursion depth. */
    JZ_LIMIT_PARSER_STATEMENT_DEPTH,       /**< Maximum parser statement recursion depth. */
    JZ_LIMIT_CONST_EVAL_DEPTH,             /**< Maximum constant-evaluator recursion depth. */
    JZ_LIMIT_IR_EXPR_DEPTH,                /**< Maximum IR expression traversal depth. */
    JZ_LIMIT_IR_STATEMENT_DEPTH,           /**< Maximum IR statement traversal depth. */
    JZ_LIMIT_AST_DEPTH,                    /**< Maximum AST traversal depth. */
    JZ_LIMIT_TEMPLATE_EXPAND_DEPTH,        /**< Maximum template-expansion recursion depth. */
    JZ_LIMIT_TRISTATE_DEPTH,               /**< Maximum tri-state analysis recursion depth. */
    JZ_LIMIT_SEM_RECURSION_DEPTH,          /**< Maximum semantic-analysis recursion depth. */
    JZ_LIMIT_REPORT_RECURSION_DEPTH,       /**< Maximum report-generation recursion depth. */
    JZ_LIMIT_CHIP_VARIANT_TUPLES,          /**< Maximum tuple count used in chip variant coverage checks. */
    JZ_LIMIT_IR_EXPANDED_ITEMS,            /**< Maximum aggregate item count created by IR expansion. */
    JZ_LIMIT_SEM_BRANCH_STATES,            /**< Maximum temporary branch states during semantic analysis. */
    JZ_LIMIT_SIM_INSTANCE_DEPTH,           /**< Maximum recursive simulation instance depth. */
    JZ_LIMIT_SIM_MEMORY_DEPTH,             /**< Maximum simulated memory depth. */
    JZ_LIMIT_SIM_MEMORY_OBJECT_BYTES,      /**< Maximum bytes for one simulated memory object. */
    JZ_LIMIT_EMITTED_TRACE_BYTES,          /**< Maximum emitted trace size in bytes. */
    JZ_LIMIT_RTLIL_MEM_INIT_EMIT_BYTES     /**< Maximum emitted RTLIL bytes for memory init cells. */
} JZInputLimitKind;

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
 * @param a   Left operand.
 * @param b   Right operand.
 * @param out Receives the sum on success.
 * @return 0 on success, -1 on overflow or invalid output pointer.
 */
int jz_size_add_checked(size_t a, size_t b, size_t *out);

/**
 * @brief Multiply two size_t values with overflow checking.
 * @param a   Left operand.
 * @param b   Right operand.
 * @param out Receives the product on success.
 * @return 0 on success, -1 on overflow or invalid output pointer.
 */
int jz_size_mul_checked(size_t a, size_t b, size_t *out);

/**
 * @brief Compute `a * b + c` with overflow checking.
 * @param a   First multiplicand.
 * @param b   Second multiplicand.
 * @param c   Value added after multiplication.
 * @param out Receives the result on success.
 * @return 0 on success, -1 on overflow or invalid output pointer.
 */
int jz_size_mul_add_checked(size_t a, size_t b, size_t c, size_t *out);

/**
 * @brief Round a size up to the next multiple of alignment.
 * @param size      Value to round up.
 * @param alignment Required power-of-two alignment.
 * @param out       Receives the aligned size on success.
 * @return 0 on success, -1 on overflow, invalid output pointer, or zero/non-power-of-two alignment.
 */
int jz_size_align_up_checked(size_t size, size_t alignment, size_t *out);

/**
 * @brief Grow a capacity using doubling arithmetic until it reaches a minimum.
 * @param current Current capacity (0 means uninitialized).
 * @param minimum Minimum required capacity.
 * @param initial Initial capacity to use when current is 0.
 * @param out Receives the grown capacity.
 * @return 0 on success, -1 on overflow or invalid input.
 */
int jz_size_grow_doubling_checked(size_t current,
                                  size_t minimum,
                                  size_t initial,
                                  size_t *out);
int jz_limit_accumulate_checked(size_t current,
                                size_t add,
                                JZInputLimitKind kind,
                                size_t *out);

/**
 * @brief Return the central hard limit value for a named input/resource policy.
 * @param kind Limit identifier.
 * @return Limit value in bytes/items/levels depending on the policy kind.
 */
size_t jz_input_limit_value(JZInputLimitKind kind);

/**
 * @brief Enter a depth-limited recursive scope.
 * @param depth Mutable depth counter.
 * @param kind Named depth-limit policy.
 * @return 0 on success, -1 if the limit would be exceeded or arguments are invalid.
 */
int jz_depth_enter_checked(unsigned *depth, JZInputLimitKind kind);

/**
 * @brief Leave a depth-limited recursive scope.
 * @param depth Mutable depth counter.
 */
void jz_depth_leave(unsigned *depth);

/**
 * @struct JZBuffer
 * @brief A growable dynamic byte buffer.
 */
typedef struct JZBuffer {
    unsigned char *data; /**< Pointer to the buffer storage, or NULL when empty. */
    size_t         len;  /**< Number of valid bytes currently stored. */
    size_t         cap;  /**< Allocated capacity in bytes. */
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
 * @param target Final output path that the temporary file will replace.
 * @param out Receives a writable stream for the temporary file on success.
 * @param tmp_path Receives the actual temporary path on success.
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
 * @param tmp_path Temporary path returned by jz_open_exclusive_temp_output().
 * @param final_path Final destination path.
 * @return 0 on success, -1 on failure.
 */
int jz_commit_exclusive_temp_output(FILE *out,
                                    const char *tmp_path,
                                    const char *final_path);

/**
 * @brief Create a unique sidecar file path using exclusive creation.
 * @param prefix Sidecar filename prefix, including any directory portion.
 * @param suffix Sidecar filename suffix, including the leading dot when needed.
 * @param out Receives a writable stream for the sidecar file on success.
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
