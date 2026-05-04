/**
 * @file driver_control.c
 * @brief Semantic checks for IF and SELECT control-flow structure.
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

#include "sem_driver.h"
#include "sem.h"
#include "util.h"
#include "rules.h"
#include "driver_internal.h"

/**
 * @brief Look up a qualified GLOBAL constant declaration.
 * @param qname Qualified name in `global.const` form.
 * @param project_symbols Project-level symbols used for lookup.
 * @return Matching CONST declaration, or `NULL` when none exists.
 */
static const JZASTNode *sem_find_global_const_decl(const char *qname,
                                                   const JZBuffer *project_symbols);
/**
 * @brief Evaluate a CASE value as an unsigned constant.
 * @param val CASE value expression.
 * @param mod_scope Module scope used for local CONST lookup.
 * @param project_symbols Project-level symbols used for GLOBAL and CONFIG lookup.
 * @param out_value Receives the evaluated value on success.
 * @return Non-zero on success, or zero when the value is not statically known.
 */
static int sem_eval_select_case_numeric_value(JZASTNode *val,
                                              const JZModuleScope *mod_scope,
                                              const JZBuffer *project_symbols,
                                              unsigned *out_value);
/**
 * @brief Check whether CASE keys cover every value of a selector width.
 * @param keys Collected CASE keys.
 * @param selector_width Bit width of the SELECT expression.
 * @return Non-zero when coverage is complete, or zero otherwise.
 */
static int sem_select_keys_cover_all_values(const JZBuffer *keys,
                                            unsigned selector_width);
/**
 * @brief Enforce width-1 IF and ELIF conditions.
 * @param stmt IF or ELIF statement to validate.
 * @param mod_scope Module scope containing the statement.
 * @param project_symbols Project-level symbols used for expression inference.
 * @param diagnostics Diagnostic sink for reported issues.
 */
static void sem_check_if_cond_width(JZASTNode *stmt,
                                    const JZModuleScope *mod_scope,
                                    const JZBuffer *project_symbols,
                                    JZDiagnosticList *diagnostics);
/**
 * @brief Validate SELECT and CASE structural control-flow rules.
 * @param select_stmt SELECT statement to validate.
 * @param mod_scope Module scope containing the statement.
 * @param project_symbols Project-level symbols used during expression checks.
 * @param is_async Non-zero when the surrounding block is asynchronous.
 * @param is_sync Non-zero when the surrounding block is synchronous.
 * @param diagnostics Diagnostic sink for reported issues.
 */
static void sem_check_select_stmt_control_flow(JZASTNode *select_stmt,
                                               const JZModuleScope *mod_scope,
                                               const JZBuffer *project_symbols,
                                               int is_async,
                                               int is_sync,
                                               JZDiagnosticList *diagnostics);
/**
 * @brief Dispatch control-flow checks for a statement subtree.
 * @param stmt Statement subtree to validate.
 * @param mod_scope Module scope containing the subtree.
 * @param project_symbols Project-level symbols used during expression checks.
 * @param is_async Non-zero when the surrounding block is asynchronous.
 * @param is_sync Non-zero when the surrounding block is synchronous.
 * @param in_conditional Non-zero when the subtree is inside conditional control flow.
 * @param diagnostics Diagnostic sink for reported issues.
 */
static void sem_check_control_flow_stmt(JZASTNode *stmt,
                                        const JZModuleScope *mod_scope,
                                        const JZBuffer *project_symbols,
                                        int is_async,
                                        int is_sync,
                                        int in_conditional,
                                        JZDiagnosticList *diagnostics);

/** @brief Captures one CASE key for SELECT coverage analysis. */
typedef struct JZSelectCaseKey {
    const char *repr;     /**< Literal text or identifier spelling. */
    JZLocation loc;       /**< Location of the CASE value expression. */
    int has_numeric;      /**< Non-zero when `numeric_value` is valid. */
    unsigned numeric_value;/**< Parsed numeric value for the CASE key. */
} JZSelectCaseKey;

static const JZASTNode *sem_find_global_const_decl(const char *qname,
                                                   const JZBuffer *project_symbols)
{
    if (!qname || !project_symbols || !project_symbols->data) return NULL;

    const char *dot = strchr(qname, '.');
    if (!dot || dot == qname || dot[1] == '\0' || strchr(dot + 1, '.')) {
        return NULL;
    }

    char ns[128];
    size_t ns_len = (size_t)(dot - qname);
    if (ns_len == 0 || ns_len >= sizeof(ns)) return NULL;
    memcpy(ns, qname, ns_len);
    ns[ns_len] = '\0';

    const char *const_name = dot + 1;
    const JZSymbol *syms = (const JZSymbol *)project_symbols->data;
    size_t count = project_symbols->len / sizeof(JZSymbol);
    for (size_t i = 0; i < count; ++i) {
        const JZSymbol *sym = &syms[i];
        if (!sym->name || sym->kind != JZ_SYM_GLOBAL ||
            strcmp(sym->name, ns) != 0 || !sym->node ||
            sym->node->type != JZ_AST_GLOBAL_BLOCK) {
            continue;
        }

        const JZASTNode *glob = sym->node;
        for (size_t j = 0; j < glob->child_count; ++j) {
            const JZASTNode *decl = glob->children[j];
            if (decl && decl->type == JZ_AST_CONST_DECL && decl->name &&
                strcmp(decl->name, const_name) == 0) {
                return decl;
            }
        }
    }

    return NULL;
}

static int sem_eval_select_case_numeric_value(JZASTNode *val,
                                              const JZModuleScope *mod_scope,
                                              const JZBuffer *project_symbols,
                                              unsigned *out_value)
{
    if (!val || !out_value) return 0;

    if (val->type == JZ_AST_EXPR_LITERAL && val->text) {
        return parse_literal_unsigned_value(val->text, out_value);
    }

    if ((val->type == JZ_AST_EXPR_IDENTIFIER ||
         val->type == JZ_AST_EXPR_QUALIFIED_IDENTIFIER) &&
        val->name) {
        if (val->type == JZ_AST_EXPR_QUALIFIED_IDENTIFIER) {
            const JZASTNode *decl =
                sem_find_global_const_decl(val->name, project_symbols);
            if (decl && decl->text) {
                if (parse_literal_unsigned_value(decl->text, out_value)) {
                    return 1;
                }
                long long global_eval = 0;
                if (sem_eval_const_expr_in_project(decl->text,
                                                   project_symbols,
                                                   &global_eval) == 0 &&
                    global_eval >= 0 && global_eval <= UINT_MAX) {
                    *out_value = (unsigned)global_eval;
                    return 1;
                }
            }
        }

        long long eval = 0;
        if (mod_scope &&
            sem_eval_const_expr_in_module(val->name,
                                          mod_scope,
                                          project_symbols,
                                          &eval) == 0 &&
            eval >= 0 && eval <= UINT_MAX) {
            *out_value = (unsigned)eval;
            return 1;
        }
    }

    return 0;
}

static int sem_select_keys_cover_all_values(const JZBuffer *keys,
                                            unsigned selector_width)
{
    if (!keys || selector_width == 0 || selector_width > 20) return 0;

    size_t total = (size_t)1u << selector_width;
    unsigned char *seen = (unsigned char *)calloc(total, sizeof(unsigned char));
    if (!seen) return 0;

    size_t covered = 0;
    const JZSelectCaseKey *arr = (const JZSelectCaseKey *)keys->data;
    size_t key_count = keys->len / sizeof(JZSelectCaseKey);
    for (size_t i = 0; i < key_count; ++i) {
        if (!arr[i].has_numeric || (size_t)arr[i].numeric_value >= total) {
            continue;
        }
        if (seen[arr[i].numeric_value]) {
            continue;
        }
        seen[arr[i].numeric_value] = 1;
        covered++;
        if (covered == total) {
            free(seen);
            return 1;
        }
    }

    free(seen);
    return 0;
}

static void sem_check_if_cond_width(JZASTNode *stmt,
                                    const JZModuleScope *mod_scope,
                                    const JZBuffer *project_symbols,
                                    JZDiagnosticList *diagnostics)
{
    if (!stmt || !mod_scope || !diagnostics) return;
    if (stmt->child_count == 0) return;
    JZASTNode *cond = stmt->children[0];
    if (!cond) return;

    JZBitvecType cond_t;
    infer_expr_type(cond, mod_scope, project_symbols, diagnostics, &cond_t);
    if (cond_t.width > 0 && cond_t.width != 1u) {
        char explain[256];
        snprintf(explain, sizeof(explain),
                 "IF/ELIF condition has width [%u] but must be width [1].\n"
                 "Use a comparison operator (==, !=) or reduction to produce\n"
                 "a 1-bit result.",
                 cond_t.width);
        sem_report_rule(diagnostics,
                        cond->loc,
                        "IF_COND_WIDTH_NOT_1",
                        explain);
    }
}

static void sem_check_select_stmt_control_flow(JZASTNode *select_stmt,
                                               const JZModuleScope *mod_scope,
                                               const JZBuffer *project_symbols,
                                               int is_async,
                                               int is_sync,
                                               JZDiagnosticList *diagnostics)
{
    if (!select_stmt || select_stmt->type != JZ_AST_STMT_SELECT || !diagnostics) {
        return;
    }

    /* child[0] is the selector expression; remaining children are CASE/DEFAULT. */
    if (select_stmt->child_count < 2) {
        return;
    }

    /* SELECT_CASE_WIDTH_MISMATCH: infer selector width and compare against CASE values.
     * Only flag when CASE value has an explicitly sized literal (contains tick mark),
     * since unsized literals (bare integers) are implicitly cast.
     */
    JZBitvecType sel_t;
    sel_t.width = 0;
    sel_t.is_signed = 0;
    if (mod_scope) {
        JZASTNode *selector = select_stmt->children[0];
        if (selector) {
            infer_expr_type(selector, mod_scope, project_symbols, diagnostics, &sel_t);
        }
        if (sel_t.width > 0) {
            for (size_t i = 1; i < select_stmt->child_count; ++i) {
                JZASTNode *child = select_stmt->children[i];
                if (!child || child->type != JZ_AST_STMT_CASE) continue;
                if (child->child_count == 0) continue;
                JZASTNode *val = child->children[0];
                if (!val) continue;
                /* Skip unsized (bare integer) literals: they're implicitly cast. */
                if (val->type == JZ_AST_EXPR_LITERAL && val->text &&
                    !strchr(val->text, '\'')) {
                    continue;
                }
                JZBitvecType case_t;
                case_t.width = 0;
                case_t.is_signed = 0;
                infer_expr_type(val, mod_scope, project_symbols, diagnostics, &case_t);
                if (case_t.width > 0 && case_t.width != sel_t.width) {
                    char explain[256];
                    const char *case_repr = (val->text) ? val->text :
                                            (val->name) ? val->name : "?";
                    snprintf(explain, sizeof(explain),
                             "CASE value '%s' has width [%u] but the selector has width [%u].\n"
                             "CASE value width must match the SELECT expression width.",
                             case_repr, case_t.width, sel_t.width);
                    sem_report_rule(diagnostics,
                                    val->loc,
                                    "SELECT_CASE_WIDTH_MISMATCH",
                                    explain);
                }
            }
        }
    }

    int has_default = 0;
    JZBuffer keys = {0}; /* array of JZSelectCaseKey */

    for (size_t i = 1; i < select_stmt->child_count; ++i) {
        JZASTNode *child = select_stmt->children[i];
        if (!child) continue;

        if (child->type == JZ_AST_STMT_DEFAULT) {
            has_default = 1;
            continue;
        }

        if (child->type != JZ_AST_STMT_CASE) {
            continue;
        }
        if (child->child_count == 0) {
            continue; /* malformed or empty CASE; parser should have rejected */
        }

        JZASTNode *val = child->children[0];
        if (!val) continue;

        const char *repr = NULL;
        if (val->type == JZ_AST_EXPR_LITERAL && val->text) {
            repr = val->text;
        } else if ((val->type == JZ_AST_EXPR_IDENTIFIER ||
                    val->type == JZ_AST_EXPR_QUALIFIED_IDENTIFIER) &&
                   val->name) {
            repr = val->name;
        }

        if (!repr) {
            continue; /* non-constant expressions are ignored for duplicate detection */
        }

        unsigned numeric_value = 0;
        int has_numeric = sem_eval_select_case_numeric_value(val,
                                                             mod_scope,
                                                             project_symbols,
                                                             &numeric_value);

        /* Check against previously seen CASE values in this SELECT. */
        JZSelectCaseKey *arr = (JZSelectCaseKey *)keys.data;
        size_t key_count = keys.len / sizeof(JZSelectCaseKey);
        for (size_t k = 0; k < key_count; ++k) {
            int duplicate = 0;
            if (has_numeric && arr[k].has_numeric) {
                duplicate = (arr[k].numeric_value == numeric_value);
            } else if (!has_numeric && !arr[k].has_numeric && arr[k].repr) {
                duplicate = (strcmp(arr[k].repr, repr) == 0);
            }

            if (duplicate) {
                char explain[256];
                snprintf(explain, sizeof(explain),
                         "CASE value '%s' appears more than once in this SELECT.\n"
                         "Only the first matching CASE executes; duplicates are dead code.",
                         repr);
                sem_report_rule(diagnostics,
                                val->loc,
                                "SELECT_DUP_CASE_VALUE",
                                explain);
                break;
            }
        }

        JZSelectCaseKey key;
        key.repr = repr;
        key.loc = val->loc;
        key.has_numeric = has_numeric;
        key.numeric_value = numeric_value;
        (void)jz_buf_append(&keys, &key, sizeof(key));
    }

    /* DEFAULT coverage diagnostics differ between ASYNCHRONOUS and SYNCHRONOUS. */
    if (!has_default) {
        if (is_async) {
            /* Count distinct CASE labels to detect genuinely incomplete
             * coverage.  If the selector width is known and the number of
             * CASE labels is less than 2^width, the select is provably
             * incomplete.
             */
            int incomplete = 0;
            if (sel_t.width > 0 && sel_t.width <= 20) {
                if (!sem_select_keys_cover_all_values(&keys, sel_t.width)) {
                    incomplete = 1;
                }
            } else {
                /* Unknown width or very wide – assume incomplete when there
                 * is no DEFAULT.
                 */
                incomplete = 1;
            }
            if (incomplete) {
                sem_report_rule(diagnostics,
                                select_stmt->loc,
                                "WARN_INCOMPLETE_SELECT_ASYNC",
                                "ASYNCHRONOUS SELECT has incomplete coverage and no DEFAULT.\n"
                                "When no CASE matches, driven nets receive no assignment,\n"
                                "which creates unintended latches. Add missing CASEs or DEFAULT.");
            } else {
                sem_report_rule(diagnostics,
                                select_stmt->loc,
                                "SELECT_DEFAULT_RECOMMENDED_ASYNC",
                                "ASYNCHRONOUS SELECT has no DEFAULT branch. Coverage appears\n"
                                "complete but a DEFAULT is still recommended for readability.");
            }
        } else if (is_sync) {
            sem_report_rule(diagnostics,
                            select_stmt->loc,
                            "SELECT_NO_MATCH_SYNC_OK",
                            "SYNCHRONOUS SELECT has no DEFAULT branch. When no CASE\n"
                            "matches, registers retain their current value (implicit hold).\n"
                            "This is legal but a DEFAULT may improve readability.");
        }
    }

    jz_buf_free(&keys);
}

static void sem_check_control_flow_stmt(JZASTNode *stmt,
                                        const JZModuleScope *mod_scope,
                                        const JZBuffer *project_symbols,
                                        int is_async,
                                        int is_sync,
                                        int in_conditional,
                                        JZDiagnosticList *diagnostics)
{
    if (!stmt || !mod_scope) return;

    /* Enforce that alias operators are only used for unconditional net aliasing.
     * Any alias assignment that appears lexically inside IF/ELIF/ELSE/SELECT/CASE
     * bodies in an ASYNCHRONOUS block is rejected.
     */
    if (in_conditional && stmt->type == JZ_AST_STMT_ASSIGN && stmt->child_count >= 2) {
        const char *op = stmt->block_kind ? stmt->block_kind : "";
        int is_alias = (strcmp(op, "ALIAS") == 0 ||
                        strcmp(op, "ALIAS_Z") == 0 ||
                        strcmp(op, "ALIAS_S") == 0);
        /* In SYNCHRONOUS blocks aliasing is already banned via SYNC_NO_ALIAS.
         * To avoid redundant diagnostics, enforce ASYNC_ALIAS_IN_CONDITIONAL
         * only for ASYNCHRONOUS contexts.
         */
        if (is_alias && is_async) {
            char explain[256];
            snprintf(explain, sizeof(explain),
                     "alias operator '%s' inside a conditional branch; did you mean '<='?\n"
                     "Aliases (=, =z, =s) create permanent wire connections and cannot\n"
                     "be conditional. Use '<=' (receive) or '=>' (drive) instead.",
                     op);
            sem_report_rule(diagnostics,
                            stmt->loc,
                            "ASYNC_ALIAS_IN_CONDITIONAL",
                            explain);
        }
    }

    switch (stmt->type) {
    case JZ_AST_STMT_IF:
    case JZ_AST_STMT_ELIF:
        /* child[0] is the condition; remaining children form the body. */
        sem_check_if_cond_width(stmt, mod_scope, project_symbols, diagnostics);
        for (size_t j = 1; j < stmt->child_count; ++j) {
            JZASTNode *body = stmt->children[j];
            if (!body) continue;
            sem_check_control_flow_stmt(body, mod_scope, project_symbols, is_async, is_sync, 1, diagnostics);
        }
        break;

    case JZ_AST_STMT_ELSE:
        for (size_t j = 0; j < stmt->child_count; ++j) {
            JZASTNode *body = stmt->children[j];
            if (!body) continue;
            sem_check_control_flow_stmt(body, mod_scope, project_symbols, is_async, is_sync, 1, diagnostics);
        }
        break;

    case JZ_AST_STMT_SELECT:
        sem_check_select_stmt_control_flow(stmt, mod_scope, project_symbols, is_async, is_sync, diagnostics);
        /* Recurse into CASE/DEFAULT bodies for nested control-flow. */
        for (size_t j = 1; j < stmt->child_count; ++j) {
            JZASTNode *branch = stmt->children[j];
            if (!branch) continue;
            sem_check_control_flow_stmt(branch, mod_scope, project_symbols, is_async, is_sync, 1, diagnostics);
        }
        break;

    case JZ_AST_STMT_CASE:
        /* child[0] is label; remaining children are body statements. */
        for (size_t j = 1; j < stmt->child_count; ++j) {
            JZASTNode *body = stmt->children[j];
            if (!body) continue;
            sem_check_control_flow_stmt(body, mod_scope, project_symbols, is_async, is_sync, 1, diagnostics);
        }
        break;

    case JZ_AST_STMT_DEFAULT:
        for (size_t j = 0; j < stmt->child_count; ++j) {
            JZASTNode *body = stmt->children[j];
            if (!body) continue;
            sem_check_control_flow_stmt(body, mod_scope, project_symbols, is_async, is_sync, 1, diagnostics);
        }
        break;

    default:
        /* Generic recursion into children to catch nested IF/SELECT constructs
         * that appear inside expression trees or other statement forms.
         */
        for (size_t j = 0; j < stmt->child_count; ++j) {
            JZASTNode *child = stmt->children[j];
            if (!child) continue;
            sem_check_control_flow_stmt(child, mod_scope, project_symbols, is_async, is_sync, in_conditional, diagnostics);
        }
        break;
    }
}

void sem_check_block_control_flow(JZASTNode *block,
                                         const JZModuleScope *mod_scope,
                                         const JZBuffer *project_symbols,
                                         int is_async,
                                         int is_sync,
                                         JZDiagnosticList *diagnostics)
{
    if (!block || !mod_scope) return;

    for (size_t i = 0; i < block->child_count; ++i) {
        JZASTNode *stmt = block->children[i];
        if (!stmt) continue;
        sem_check_control_flow_stmt(stmt, mod_scope, project_symbols, is_async, is_sync, 0, diagnostics);
    }
}
