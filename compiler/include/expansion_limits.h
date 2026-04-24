#ifndef JZ_HDL_EXPANSION_LIMITS_H
#define JZ_HDL_EXPANSION_LIMITS_H

#include <stddef.h>

typedef struct JZExpansionLimits {
    size_t repeat_max_count;
    size_t repeat_max_bytes;
    size_t apply_max_count;
    size_t apply_max_growth;
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
