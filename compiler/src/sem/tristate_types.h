/**
 * @file tristate_types.h
 * @brief Shared tri-state analysis result types.
 */

#ifndef JZ_HDL_TRISTATE_TYPES_H
#define JZ_HDL_TRISTATE_TYPES_H

#include "ast.h"
#include "util.h"

/* -------------------------------------------------------------------------
 *  Tri-State Analysis Data Structures
 *
 *  Shared between the proof engine (driver_tristate.c) and the report
 *  formatter (tristate_report.c).
 * -------------------------------------------------------------------------
 */

/** Result classification for tri-state resolution analysis. */
typedef enum JZTristateResult {
    JZ_TRISTATE_PROVEN,    /**< Mutual exclusion was proven. */
    JZ_TRISTATE_DISPROVEN, /**< A conflicting driver combination was found. */
    JZ_TRISTATE_UNKNOWN    /**< The analysis could not reach a proof result. */
} JZTristateResult;

/** Reason codes for an unknown tri-state result. */
typedef enum JZTristateUnknownReason {
    JZ_TRISTATE_UNKNOWN_BLACKBOX,            /**< A blackbox module output prevents proof. */
    JZ_TRISTATE_UNKNOWN_UNCONSTRAINED_INPUT, /**< A primary input is not constrained. */
    JZ_TRISTATE_UNKNOWN_COMPLEX_EXPR,        /**< A guard or driver expression is too complex. */
    JZ_TRISTATE_UNKNOWN_MULTI_CLOCK,         /**< Drivers span multiple clock domains. */
    JZ_TRISTATE_UNKNOWN_NO_GUARD             /**< No usable guard condition was found. */
} JZTristateUnknownReason;

/** Method used to prove mutual exclusion. */
typedef enum JZTristateProofMethod {
    JZ_TRISTATE_PROOF_SINGLE_DRIVER,        /**< The net has only one driver. */
    JZ_TRISTATE_PROOF_DISTINCT_CONSTANTS,   /**< Guards compare the same input to distinct constants. */
    JZ_TRISTATE_PROOF_COMPLEMENTARY_GUARDS, /**< Guards are logical complements. */
    JZ_TRISTATE_PROOF_SINGLE_NON_Z,         /**< At most one driver can produce a non-`z` value. */
    JZ_TRISTATE_PROOF_IF_ELSE_BRANCHES,     /**< Drivers occupy complementary `if` and `else` branches. */
    JZ_TRISTATE_PROOF_PAIRWISE              /**< Every driver pair was proven mutually exclusive. */
} JZTristateProofMethod;

/* Maximum number of IF/ELSE guard terms stored per driver. */
#define JZ_MAX_GUARD_TERMS 8

/* Maximum number of comparison terms extracted from an AND chain. */
#define JZ_MAX_COMPARE_TERMS 8

/* Maximum number of alias entries stored per driver. */
#define JZ_MAX_DRIVER_ALIASES 8

/** One comparison term extracted from a guard condition. */
typedef struct JZCompareTerm {
    const char *input_name;    /**< Signal name, such as `pbus.CMD`. */
    const char *compare_value; /**< Compared value, such as `CMD.READ`. */
    int         is_inverted;   /**< Non-zero when the comparison is inverted. */
} JZCompareTerm;

/** Enable condition extracted from a tri-state driver. */
typedef struct JZDriverEnable {
    const char *input_name;        /**< Input identifier referenced by the guard. */
    const char *compare_value;     /**< Literal or constant compared against the input. */
    int         is_inverted;       /**< Non-zero when the guard is logically inverted. */
    int         is_complex;        /**< Non-zero when the guard could not be reduced to a simple form. */
    char        normalized_lhs[64]; /**< Display-ready left-hand side text. */
    char        normalized_rhs[64]; /**< Display-ready right-hand side text. */
    char        condition_text[256]; /**< Full human-readable guard text. */
    JZLocation  loc;               /**< Source location of the guard. */
    size_t      n_guard_terms;     /**< Number of stored `if`/`else` guard terms. */
    struct {
        const void *expr; /**< Condition expression node stored as an opaque AST pointer. */
        int         neg;  /**< Non-zero when the stored branch is negated. */
    } guard_terms[JZ_MAX_GUARD_TERMS]; /**< Extracted branch guard terms. */
    size_t         n_compare_terms; /**< Number of extracted comparison terms. */
    JZCompareTerm  compare_terms[JZ_MAX_COMPARE_TERMS]; /**< Comparison terms from guard conjunctions. */
} JZDriverEnable;

/** Information about one driver of a tri-state net. */
typedef struct JZTristateDriver {
    JZASTNode      *stmt;              /**< Driver statement or instance node. */
    JZLocation      loc;               /**< Source location of the driver. */
    int             can_produce_z;     /**< Non-zero when the driver can output high impedance. */
    int             can_produce_non_z; /**< Non-zero when the driver can output a non-`z` value. */
    JZDriverEnable  enable;            /**< Extracted enable condition details. */
    const char     *source_snippet;    /**< Optional source snippet for reporting. */
    const char     *instance_name;     /**< Instance name when the driver comes from a module instance. */
    const char     *module_name;       /**< Module name when the driver comes from a module instance. */
    const char     *port_name;         /**< Port name when the driver is bound through a port. */
    size_t          n_aliases;         /**< Number of stored alias mappings. */
    struct {
        const char *from; /**< Source alias name, such as `bus_cmd`. */
        const char *to;   /**< Resolved alias target, such as `pbus.CMD`. */
    } aliases[JZ_MAX_DRIVER_ALIASES]; /**< Alias mappings visible to the driver. */
} JZTristateDriver;

/** Information about one sink of a tri-state net. */
typedef struct JZTristateSink {
    JZASTNode  *stmt;    /**< Sink statement node. */
    JZLocation  loc;     /**< Source location of the sink. */
    const char *snippet; /**< Optional source snippet for reporting. */
} JZTristateSink;

/** Witness describing one conflicting driver pair. */
typedef struct JZConflictWitness {
    size_t     driver_a; /**< Index of the first conflicting driver. */
    size_t     driver_b; /**< Index of the second conflicting driver. */
    const char *reason;  /**< Human-readable explanation of the conflict. */
} JZConflictWitness;

/** Complete tri-state analysis result for one net. */
typedef struct JZTristateNetInfo {
    const char        *name;            /**< Net name. */
    unsigned           width;           /**< Net bit width. */
    JZLocation         decl_loc;        /**< Declaration location for the net. */
    JZBuffer           drivers;         /**< Buffer of `JZTristateDriver` entries. */
    JZBuffer           sinks;           /**< Buffer of `JZTristateSink` entries. */
    JZTristateResult   result;          /**< Final proof classification. */
    JZTristateProofMethod proof_method; /**< Proof method used when the result is proven. */
    JZTristateUnknownReason unknown_reason; /**< Reason code used when the result is unknown. */
    JZConflictWitness  conflict;        /**< Conflict witness used when the result is disproven. */
} JZTristateNetInfo;

/** Expanded guard condition information used during analysis. */
typedef struct JZTristateGuardInfo {
    const char *input_name; /**< Input identifier referenced by the guard. */
    const char *compare_lit; /**< Compared literal or constant text. */
    int         is_inverted; /**< Non-zero when the effective guard is inverted. */
    char        normalized_lhs[64]; /**< Display-ready left-hand side text. */
    char        normalized_rhs[64]; /**< Display-ready right-hand side text. */
    char        condition_text[256]; /**< Full human-readable guard text. */
    size_t      n_guard_terms; /**< Number of stored branch guard terms. */
    struct {
        const JZASTNode *expr; /**< Condition expression node. */
        int              neg;  /**< Non-zero when the stored branch is negated. */
    } guard_terms[JZ_MAX_GUARD_TERMS]; /**< Extracted branch guard terms. */
    size_t         n_compare_terms; /**< Number of extracted comparison terms. */
    JZCompareTerm  compare_terms[JZ_MAX_COMPARE_TERMS]; /**< Comparison terms from guard conjunctions. */
    size_t         n_aliases; /**< Number of stored alias mappings. */
    struct {
        const char *from; /**< Source alias name. */
        const char *to;   /**< Resolved alias target. */
    } aliases[JZ_MAX_DRIVER_ALIASES]; /**< Alias mappings visible in the guard context. */
} JZTristateGuardInfo;

#endif /* JZ_HDL_TRISTATE_TYPES_H */
