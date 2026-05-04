/**
 * @file chip_data_internal.h
 * @brief Internal chip-data JSON helpers shared across chip-data loaders.
 */

#ifndef JZ_HDL_CHIP_DATA_INTERNAL_H
#define JZ_HDL_CHIP_DATA_INTERNAL_H

#include "third_party/jsmn.h"

/**
 * @brief Return the built-in JSON payload for a known chip identifier.
 *
 * @param chip_id Chip identifier to look up.
 * @return Pointer to the built-in JSON text, or NULL when the chip is unknown.
 */
const char *jz_chip_builtin_json(const char *chip_id);

/**
 * @brief Check whether a JSON token matches a string exactly.
 *
 * @param json Full JSON source buffer.
 * @param tok  Token to compare.
 * @param s    Expected string value.
 * @return Non-zero when the token text matches `s`, otherwise 0.
 */
int jz_json_token_eq(const char *json, const jsmntok_t *tok, const char *s);

/**
 * @brief Check whether a JSON token matches a string case-insensitively.
 *
 * @param json Full JSON source buffer.
 * @param tok  Token to compare.
 * @param s    Expected string value.
 * @return Non-zero when the token text matches `s` case-insensitively, otherwise 0.
 */
int jz_json_token_eq_ci(const char *json, const jsmntok_t *tok, const char *s);

/**
 * @brief Skip a token subtree in a flat JSMN token array.
 *
 * @param toks  Token array.
 * @param count Number of entries in `toks`.
 * @param index Starting token index.
 * @return Index of the next token after the skipped subtree, or `index` on failure.
 */
int jz_json_skip(const jsmntok_t *toks, int count, int index);

/**
 * @brief Parse an unsigned integer from a JSON token.
 *
 * @param json Full JSON source buffer.
 * @param tok  Token containing the numeric text.
 * @param out  Receives the parsed value on success.
 * @return Non-zero on success, otherwise 0.
 */
int jz_json_token_to_uint(const char *json, const jsmntok_t *tok, unsigned *out);

/**
 * @brief Duplicate a JSON token's text into heap storage.
 *
 * @param json Full JSON source buffer.
 * @param tok  Token whose text should be copied.
 * @return Newly allocated NUL-terminated string, or NULL on allocation failure.
 */
char *jz_json_token_strdup(const char *json, const jsmntok_t *tok);

#endif /* JZ_HDL_CHIP_DATA_INTERNAL_H */
