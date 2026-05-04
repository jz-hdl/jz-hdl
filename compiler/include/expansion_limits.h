/**
 * @file expansion_limits.h
 * @brief Safety limits for source expansion passes.
 */

#ifndef JZ_HDL_EXPANSION_LIMITS_H
#define JZ_HDL_EXPANSION_LIMITS_H

#include <stddef.h>

/**
 * @struct JZExpansionLimits
 * @brief Upper bounds applied to repeat and template-apply expansion.
 */
typedef struct JZExpansionLimits {
    size_t repeat_max_count; /**< Maximum number of repeated items emitted by one expansion. */
    size_t repeat_max_bytes; /**< Maximum total bytes emitted by one repeat expansion. */
    size_t apply_max_count;  /**< Maximum number of nested or repeated template applications. */
    size_t apply_max_growth; /**< Maximum multiplicative growth allowed for one apply expansion. */
} JZExpansionLimits;

#define JZ_REPEAT_MAX_COUNT_DEFAULT 1024u
#define JZ_REPEAT_MAX_BYTES_DEFAULT (1024u * 1024u)
#define JZ_APPLY_MAX_COUNT_DEFAULT 1024u
#define JZ_APPLY_MAX_GROWTH_DEFAULT 4096u

#define JZ_EXPANSION_LIMITS_DEFAULT_INIT \
    { \
        JZ_REPEAT_MAX_COUNT_DEFAULT, \
        JZ_REPEAT_MAX_BYTES_DEFAULT, \
        JZ_APPLY_MAX_COUNT_DEFAULT, \
        JZ_APPLY_MAX_GROWTH_DEFAULT \
    }

#endif /* JZ_HDL_EXPANSION_LIMITS_H */
