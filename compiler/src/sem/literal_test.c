/**
 * @file literal_test.c
 * @brief Unit tests for literal semantic analysis helpers.
 */

#include <stdio.h>

#include "sem.h"

/**
 * @brief Check one literal analysis case.
 * @param base Numeric base passed to `jz_literal_analyze`.
 * @param value Literal value text without width or base prefix.
 * @param declared_width Declared destination width to analyze against.
 * @param expected_intrinsic Expected intrinsic width when the call succeeds.
 * @param expected_ext Expected extension kind when the call succeeds.
 * @param should_fail Non-zero when the analysis is expected to fail.
 * @return `0` on success, or `1` when the observed result differs from the expectation.
 */
static int expect_literal(JZNumericBase base,
                          const char *value,
                          unsigned declared_width,
                          unsigned expected_intrinsic,
                          JZLiteralExtKind expected_ext,
                          int should_fail)
{
    unsigned intrinsic = 0;
    JZLiteralExtKind ext = JZ_LITERAL_EXT_NONE;
    int rc = jz_literal_analyze(base, value, declared_width, &intrinsic, &ext);
    if (should_fail) {
        if (rc == 0) {
            fprintf(stderr,
                    "expected jz_literal_analyze to fail for '%s' (base=%d, width=%u) but it succeeded\n",
                    value, (int)base, declared_width);
            return 1;
        }
        return 0;
    }
    if (rc != 0) {
        fprintf(stderr, "unexpected jz_literal_analyze failure for '%s'\n", value);
        return 1;
    }
    if (intrinsic != expected_intrinsic) {
        fprintf(stderr, "intrinsic width %u, expected %u for '%s'\n",
                intrinsic, expected_intrinsic, value);
        return 1;
    }
    if (ext != expected_ext) {
        fprintf(stderr, "extension kind %d, expected %d for '%s'\n",
                (int)ext, (int)expected_ext, value);
        return 1;
    }
    return 0;
}

/**
 * @brief Run the literal analysis unit tests.
 * @return Exit status for the test executable.
 */
int main(void)
{
    int failures = 0;

    /* Binary literals: 1 bit per digit. */
    failures += expect_literal(JZ_NUM_BASE_BIN, "1010", 4, 4, JZ_LITERAL_EXT_NONE, 0);
    failures += expect_literal(JZ_NUM_BASE_BIN, "1", 4, 1, JZ_LITERAL_EXT_ZERO, 0);
    failures += expect_literal(JZ_NUM_BASE_BIN, "x", 4, 1, JZ_LITERAL_EXT_X, 0);
    failures += expect_literal(JZ_NUM_BASE_BIN, "z", 4, 1, JZ_LITERAL_EXT_Z, 0);

    /* Hex literals: intrinsic width from numeric magnitude for 0–F digits. */
    failures += expect_literal(JZ_NUM_BASE_HEX, "F", 8, 4, JZ_LITERAL_EXT_ZERO, 0);
    failures += expect_literal(JZ_NUM_BASE_HEX, "1FFFFFF", 25, 25, JZ_LITERAL_EXT_NONE, 0);
    failures += expect_literal(JZ_NUM_BASE_HEX, "3FFFFFF", 25, 0, JZ_LITERAL_EXT_NONE, 1);

    /* Decimal literals: intrinsic width from numeric value, zero-extend only. */
    failures += expect_literal(JZ_NUM_BASE_DEC, "0", 8, 1, JZ_LITERAL_EXT_ZERO, 0);
    failures += expect_literal(JZ_NUM_BASE_DEC, "10", 8, 4, JZ_LITERAL_EXT_ZERO, 0);

    /* Overflow cases should fail. */
    failures += expect_literal(JZ_NUM_BASE_BIN, "10101", 4, 0, JZ_LITERAL_EXT_NONE, 1);

    if (failures != 0) {
        fprintf(stderr, "%d literal semantics tests failed\n", failures);
        return 1;
    }

    return 0;
}
