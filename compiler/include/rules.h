/**
 * @file rules.h
 * @brief Static metadata and lookup for validation rules.
 *
 * Provides access to rule definitions loaded from rules.ini, including
 * rule grouping, severity mode, and descriptions. Rules are organized
 * by group (for example, "LITERALS_AND_TYPES") with individual IDs,
 * severities, priorities, and human-readable descriptions.
 */

#ifndef JZ_HDL_RULES_H
#define JZ_HDL_RULES_H

#include <stddef.h>
#include <stdio.h>

/**
 * @enum JZRuleMode
 * @brief Severity mode for a validation rule.
 */
typedef enum JZRuleMode {
    JZ_RULE_MODE_ERR = 0, /**< Rule is reported as an error. */
    JZ_RULE_MODE_WRN = 1, /**< Rule is reported as a warning. */
    JZ_RULE_MODE_INF = 2  /**< Rule is reported as informational output. */
} JZRuleMode;

/**
 * @struct JZRuleInfo
 * @brief Metadata for a single validation rule.
 */
typedef struct JZRuleInfo {
    const char *group;       /**< Rule group name from `rules.ini`. */
    const char *id;          /**< Stable rule identifier such as `LIT_OVERFLOW`. */
    int         priority;    /**< Relative priority used when ordering same-location diagnostics. */
    JZRuleMode  mode;        /**< Severity mode for this rule. */
    const char *description; /**< Human-readable rule description. */
} JZRuleInfo;

/**
 * @brief Table of all defined validation rules.
 */
extern const JZRuleInfo jz_rule_table[];

/**
 * @brief Number of rules in the rule table.
 */
extern const size_t jz_rule_table_count;

/**
 * @brief Look up a rule by identifier.
 *
 * @param id Rule identifier such as `LIT_OVERFLOW`.
 * @return Pointer to rule metadata, or NULL if not found.
 */
const JZRuleInfo *jz_rule_lookup(const char *id);

/**
 * @brief Print all validation rules grouped by category.
 *
 * @param out Output stream that receives the formatted rule listing.
 */
void jz_rules_print_all(FILE *out);

#endif /* JZ_HDL_RULES_H */
