/**
 * @file driver_flow.c
 * @brief Exclusive-assignment and dead-code flow analysis helpers.
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

#include "sem_driver.h"
#include "sem.h"
#include "util.h"
#include "rules.h"
#include "driver_internal.h"
#include <time.h>


/* -------------------------------------------------------------------------
 *  Constant expression evaluator for template-expanded slice bounds
 * -------------------------------------------------------------------------
 *  After @apply expansion, IDX becomes a literal but slice bounds like
 *  IDX*11+10 become EXPR_BINARY trees of literals.  This small evaluator
 *  handles the constant integer arithmetic that template expansion produces.
 */
int sem_try_const_eval_ast_expr(const JZASTNode *expr, long *out)
{
    if (!expr || !out) return 0;

    if (expr->type == JZ_AST_EXPR_LITERAL && expr->text) {
        char *end = NULL;
        long v = strtol(expr->text, &end, 0);
        if (end && end != expr->text && *end == '\0') {
            *out = v;
            return 1;
        }
        return 0;
    }

    if (expr->type == JZ_AST_EXPR_BINARY && expr->block_kind &&
        expr->child_count >= 2) {
        long lv, rv;
        if (!sem_try_const_eval_ast_expr(expr->children[0], &lv)) return 0;
        if (!sem_try_const_eval_ast_expr(expr->children[1], &rv)) return 0;

        if (strcmp(expr->block_kind, "ADD") == 0) { *out = lv + rv; return 1; }
        if (strcmp(expr->block_kind, "SUB") == 0) { *out = lv - rv; return 1; }
        if (strcmp(expr->block_kind, "MUL") == 0) { *out = lv * rv; return 1; }
        if (strcmp(expr->block_kind, "DIV") == 0) {
            if (rv == 0) return 0;
            *out = lv / rv; return 1;
        }
        if (strcmp(expr->block_kind, "MOD") == 0) {
            if (rv == 0) return 0;
            *out = lv % rv; return 1;
        }
    }

    return 0;
}

/* -------------------------------------------------------------------------
 *  Exclusive Assignment Rule (PED)
 * -------------------------------------------------------------------------
 */

/* JZAssignRange is defined in driver_internal.h */
/** @brief Key for grouping assignments to the same declaration kind. */
typedef struct JZAssignKey {
    JZASTNode *decl;      /**< Target declaration node. */
    int        is_register;/**< Non-zero when the target is a register. */
} JZAssignKey;

/** @brief One assignment record tracked on a control-flow path. */
typedef struct JZAssignRecord {
    JZAssignKey   key;        /**< Target identity for the assignment. */
    JZAssignRange range;      /**< Bit-range covered by the assignment. */
    int           is_nested;  /**< Non-zero when found in nested control flow. */
    int           partial;    /**< Non-zero when assigned on only some paths. */
    int           can_drive_z;/**< Non-zero when the assignment may drive `z`. */
    JZASTNode    *stmt;       /**< Originating assignment statement. */
} JZAssignRecord;

/** @brief Assignment-state snapshot for one control-flow path. */
typedef struct JZPathState {
    JZBuffer assigns; /**< Array of `JZAssignRecord` entries. */
    /* Number of records in `assigns` that were inherited when this path was
     * cloned for branch analysis. Records at index >= branch_base were added
     * by statements in the current branch body; records below it came from
     * outer scope (root-level or prior merged IF/SELECT chains). Used to
     * distinguish "same-branch double-assign" from "independent chain
     * conflict" when an overlap is detected. Zero on the root path. */
    size_t   branch_base; /**< Index where branch-local records begin. */
} JZPathState;

/**
 * @brief Check whether a SELECT statement covers all selector values.
 * @param select_stmt SELECT statement to inspect.
 * @param scope Module scope containing the statement.
 * @param project_symbols Project-level symbols used for evaluation.
 * @return Non-zero when the CASE set is exhaustive, or zero otherwise.
 */
static int sem_dead_select_cases_exhaustive(JZASTNode *select_stmt,
                                            const JZModuleScope *scope,
                                            const JZBuffer *project_symbols);

/**
 * @brief Check whether an expression subtree contains a `z` literal.
 * @param expr Expression subtree to inspect.
 * @return Non-zero when a `z` literal is present, or zero otherwise.
 */
static int sem_flow_expr_contains_z_literal(const JZASTNode *expr)
{
    if (!expr) return 0;

    if (expr->type == JZ_AST_EXPR_LITERAL && expr->text) {
        const char *tick = strchr(expr->text, '\'');
        if (!tick) return 0;
        const char *value = tick + 2;
        for (const char *p = value; *p; ++p) {
            if (*p == 'z' || *p == 'Z') {
                return 1;
            }
        }
        return 0;
    }

    for (size_t i = 0; i < expr->child_count; ++i) {
        if (sem_flow_expr_contains_z_literal(expr->children[i])) {
            return 1;
        }
    }
    return 0;
}

static int sem_flow_lhs_targets_decl(const JZASTNode *lhs,
                                     const JZASTNode *decl)
{
    if (!lhs || !decl || !decl->name) return 0;

    switch (lhs->type) {
    case JZ_AST_EXPR_IDENTIFIER:
        return lhs->name && strcmp(lhs->name, decl->name) == 0;

    case JZ_AST_EXPR_SLICE:
        return lhs->child_count >= 1 &&
               sem_flow_lhs_targets_decl(lhs->children[0], decl);

    case JZ_AST_EXPR_CONCAT:
        for (size_t i = 0; i < lhs->child_count; ++i) {
            if (sem_flow_lhs_targets_decl(lhs->children[i], decl)) {
                return 1;
            }
        }
        return 0;

    default:
        return 0;
    }
}

static int sem_flow_node_has_z_assignment_to_decl(const JZASTNode *node,
                                                  const JZASTNode *decl)
{
    if (!node || !decl) return 0;

    if (node->type == JZ_AST_STMT_ASSIGN && node->child_count >= 2) {
        const char *op = node->block_kind ? node->block_kind : "";
        int is_alias = (strcmp(op, "ALIAS") == 0 ||
                        strcmp(op, "ALIAS_Z") == 0 ||
                        strcmp(op, "ALIAS_S") == 0);
        int is_drive = (strncmp(op, "DRIVE", 5) == 0);
        if (!is_alias) {
            const JZASTNode *target = is_drive ? node->children[1] : node->children[0];
            const JZASTNode *rhs = node->children[1];
            if (sem_flow_lhs_targets_decl(target, decl) &&
                sem_flow_expr_contains_z_literal(rhs)) {
                return 1;
            }
        }
    }

    for (size_t i = 0; i < node->child_count; ++i) {
        if (sem_flow_node_has_z_assignment_to_decl(node->children[i], decl)) {
            return 1;
        }
    }
    return 0;
}

static void sem_excl_free_paths(JZBuffer *paths)
{
    if (!paths) return;
    size_t count = paths->len / sizeof(JZPathState);
    JZPathState *arr = (JZPathState *)paths->data;
    for (size_t i = 0; i < count; ++i) {
        jz_buf_free(&arr[i].assigns);
    }
    jz_buf_free(paths);
}

static int sem_excl_clone_path_state(const JZPathState *src,
                                     JZPathState *dst)
{
    if (!dst) return -1;
    memset(dst, 0, sizeof(*dst));
    if (!src) return 0;
    if (src->assigns.len == 0) {
        /* No inherited records; branch_base stays 0. */
        return 0;
    }
    if (jz_buf_append(&dst->assigns, src->assigns.data, src->assigns.len) != 0) {
        jz_buf_free(&dst->assigns);
        return -1;
    }
    /* Mark all inherited records as pre-branch; any records added subsequently
     * by branch-body analysis will sit at index >= branch_base. */
    dst->branch_base = dst->assigns.len / sizeof(JZAssignRecord);
    return 0;
}

/* JZAssignTargetEntry is defined in driver_internal.h */
static void sem_excl_add_target_from_symbol(const JZSymbol *sym,
                                            const JZAssignRange *range,
                                            int is_sync,
                                            int is_nested,
                                            JZBuffer *out)
{
    if (!sym || !sym->node || !out) return;

    int is_reg   = (sym->kind == JZ_SYM_REGISTER);
    int is_latch = (sym->kind == JZ_SYM_LATCH);

    if (is_sync) {
        if (!is_reg) return;
    } else {
        if (sym->kind != JZ_SYM_PORT &&
            sym->kind != JZ_SYM_WIRE &&
            sym->kind != JZ_SYM_REGISTER &&
            sym->kind != JZ_SYM_LATCH) {
            return;
        }
    }

    JZAssignTargetEntry entry;
    entry.decl        = sym->node;
    entry.is_register = (is_reg || is_latch);
    entry.range       = *range;
    entry.is_nested   = is_nested;
    (void)jz_buf_append(out, &entry, sizeof(entry));
}

void sem_excl_collect_targets_from_lhs(JZASTNode *lhs,
                                       const JZModuleScope *scope,
                                       const JZBuffer *project_symbols,
                                       int is_sync,
                                       int is_nested,
                                       JZBuffer *out)
{
    if (!lhs || !scope || !out) return;

    switch (lhs->type) {
    case JZ_AST_EXPR_CONCAT:
        for (size_t i = 0; i < lhs->child_count; ++i) {
            sem_excl_collect_targets_from_lhs(lhs->children[i], scope, project_symbols, is_sync, is_nested, out);
        }
        break;

    case JZ_AST_EXPR_SLICE: {
        if (lhs->child_count < 1) return;
        JZASTNode *base = lhs->children[0];
        JZAssignRange r;
        memset(&r, 0, sizeof(r));

        if (lhs->child_count >= 3) {
            unsigned msb = 0, lsb = 0;
            if (lhs->children[1] && lhs->children[1]->type == JZ_AST_EXPR_LITERAL && lhs->children[1]->text &&
                lhs->children[2] && lhs->children[2]->type == JZ_AST_EXPR_LITERAL && lhs->children[2]->text &&
                parse_simple_nonnegative_int(lhs->children[1]->text, &msb) &&
                parse_simple_nonnegative_int(lhs->children[2]->text, &lsb) &&
                msb >= lsb) {
                r.has_range = 1;
                r.lsb = lsb;
                r.msb = msb;
            }
            /* Fallback: evaluate constant expression trees (template-expanded IDX arithmetic) */
            if (!r.has_range && lhs->children[1] && lhs->children[2]) {
                long msb_val = 0, lsb_val = 0;
                if (sem_try_const_eval_ast_expr(lhs->children[1], &msb_val) &&
                    sem_try_const_eval_ast_expr(lhs->children[2], &lsb_val) &&
                    msb_val >= 0 && lsb_val >= 0 && msb_val >= lsb_val) {
                    r.has_range = 1;
                    r.msb = (unsigned)msb_val;
                    r.lsb = (unsigned)lsb_val;
                }
            }
        }

        if (!base || base->type != JZ_AST_EXPR_IDENTIFIER || !base->name) {
            return;
        }
        const JZSymbol *sym = module_scope_lookup(scope, base->name);
        sem_excl_add_target_from_symbol(sym, &r, is_sync, is_nested, out);
        break;
    }

    case JZ_AST_EXPR_IDENTIFIER: {
        if (!lhs->name) return;
        JZAssignRange r;
        memset(&r, 0, sizeof(r));
        const JZSymbol *sym = module_scope_lookup(scope, lhs->name);
        sem_excl_add_target_from_symbol(sym, &r, is_sync, is_nested, out);
        break;
    }

    case JZ_AST_EXPR_BUS_ACCESS:
    case JZ_AST_EXPR_QUALIFIED_IDENTIFIER: {
        if (!project_symbols) return;
        JZBusAccessInfo info;
        if (!sem_resolve_bus_access(lhs, scope, project_symbols, &info, NULL)) {
            return;
        }
        if (!info.signal_decl || !info.port_decl) {
            return;
        }
        if (is_sync) {
            return;
        }
        if (info.is_wildcard && info.count > 0) {
            for (unsigned i = 0; i < info.count; ++i) {
                JZASTNode *decl = sem_bus_get_or_create_signal_decl((JZModuleScope *)scope,
                                                                    info.port_decl->name,
                                                                    1,
                                                                    i,
                                                                    info.signal_name,
                                                                    info.signal_decl);
                if (!decl) continue;
                JZAssignTargetEntry entry;
                entry.decl = decl;
                entry.is_register = 0;
                memset(&entry.range, 0, sizeof(entry.range));
                entry.is_nested = is_nested;
                (void)jz_buf_append(out, &entry, sizeof(entry));
            }
        } else if (info.has_index && info.index_known) {
            JZASTNode *decl = sem_bus_get_or_create_signal_decl((JZModuleScope *)scope,
                                                                info.port_decl->name,
                                                                1,
                                                                info.index_value,
                                                                info.signal_name,
                                                                info.signal_decl);
            if (decl) {
                JZAssignTargetEntry entry;
                entry.decl = decl;
                entry.is_register = 0;
                memset(&entry.range, 0, sizeof(entry.range));
                entry.is_nested = is_nested;
                (void)jz_buf_append(out, &entry, sizeof(entry));
            }
        } else {
            JZASTNode *decl = sem_bus_get_or_create_signal_decl((JZModuleScope *)scope,
                                                                info.port_decl->name,
                                                                0,
                                                                0,
                                                                info.signal_name,
                                                                info.signal_decl);
            if (decl) {
                JZAssignTargetEntry entry;
                entry.decl = decl;
                entry.is_register = 0;
                memset(&entry.range, 0, sizeof(entry.range));
                entry.is_nested = is_nested;
                (void)jz_buf_append(out, &entry, sizeof(entry));
            }
        }
        break;
    }

    default:
        break;
    }
}

static void sem_excl_record_assignment_in_path(JZPathState *path,
                                               const JZAssignTargetEntry *entries,
                                               size_t entry_count,
                                               int is_sync,
                                               JZLocation loc,
                                               JZDiagnosticList *diagnostics)
{
    if (!path || !entries || entry_count == 0) return;

    JZAssignRecord *records = (JZAssignRecord *)path->assigns.data;
    size_t record_count = path->assigns.len / sizeof(JZAssignRecord);

    for (size_t ei = 0; ei < entry_count; ++ei) {
        const JZAssignTargetEntry *e = &entries[ei];

        for (size_t ri = 0; ri < record_count; ++ri) {
            JZAssignRecord *r = &records[ri];
            if (r->key.decl != e->decl || r->key.is_register != e->is_register) {
                continue;
            }

            int overlap = 0;
            if (!r->range.has_range || !e->range.has_range) {
                overlap = 1;
            } else {
                if (!(e->range.msb < r->range.lsb || e->range.lsb > r->range.msb)) {
                    overlap = 1;
                }
            }

            if (overlap) {
                const char *rule_id = NULL;
                if (!is_sync && (r->can_drive_z || e->can_drive_z)) {
                    continue;
                }

                if (is_sync && e->is_register) {
                    int root_and_conditional = ((r->is_nested && !e->is_nested) ||
                                                (!r->is_nested && e->is_nested));
                    rule_id = root_and_conditional
                        ? "SYNC_ROOT_AND_CONDITIONAL_ASSIGN"
                        : "SYNC_MULTI_ASSIGN_SAME_REG_BITS";
                } else if (r->range.has_range && e->range.has_range) {
                    rule_id = "ASSIGN_SLICE_OVERLAP";
                } else if (!is_sync) {
                    int root_and_conditional = ((r->is_nested && !e->is_nested) ||
                                                (!r->is_nested && e->is_nested));
                    /* A conflicting record added during the CURRENT branch
                     * body sits at index >= path->branch_base. Records below
                     * that were inherited from outer scope (root or prior
                     * merged IF/SELECT). This distinguishes "two assigns in
                     * one branch body" (ASSIGN_MULTIPLE_SAME_BITS) from "same
                     * target assigned in two independent chains"
                     * (ASSIGN_INDEPENDENT_IF_SELECT). */
                    int conflict_in_current_branch = (ri >= path->branch_base);
                    if (root_and_conditional) {
                        rule_id = "ASSIGN_SHADOWING";
                    } else if (!r->is_nested && !e->is_nested) {
                        /* Plain root-level multiple assign to same bits in ASYNCHRONOUS block. */
                        rule_id = "ASSIGN_MULTIPLE_SAME_BITS";
                    } else if (conflict_in_current_branch) {
                        /* Both assignments are in the same branch body — a
                         * single execution path violation, not independent
                         * chains. */
                        rule_id = "ASSIGN_MULTIPLE_SAME_BITS";
                    } else {
                        rule_id = "ASSIGN_INDEPENDENT_IF_SELECT";
                    }
                } else {
                    rule_id = "ASSIGN_MULTIPLE_SAME_BITS_FALLBACK";
                }

                sem_report_rule(diagnostics,
                                loc,
                                rule_id,
                                "multiple assignments to the same bits occur along a single execution path");
                break;
            }
        }

        JZAssignRecord rec;
        rec.key.decl        = e->decl;
        rec.key.is_register = e->is_register;
        rec.range           = e->range;
        rec.is_nested       = e->is_nested;
        rec.partial         = 0;
        rec.can_drive_z     = e->can_drive_z;
        rec.stmt            = NULL;
        (void)jz_buf_append(&path->assigns, &rec, sizeof(rec));
        records = (JZAssignRecord *)path->assigns.data;
        record_count = path->assigns.len / sizeof(JZAssignRecord);
    }
}

static void sem_excl_analyze_block(JZASTNode *block,
                                   const JZModuleScope *scope,
                                   const JZBuffer *project_symbols,
                                   int is_sync,
                                   int nesting_depth,
                                   JZBuffer *paths,
                                   JZDiagnosticList *diagnostics);

static void sem_excl_analyze_stmt(JZASTNode *stmt,
                                  const JZModuleScope *scope,
                                  const JZBuffer *project_symbols,
                                  int is_sync,
                                  int nesting_depth,
                                  JZBuffer *paths,
                                  JZDiagnosticList *diagnostics);

static void sem_excl_analyze_if_branch_body(JZASTNode *stmt,
                                            const JZModuleScope *scope,
                                            const JZBuffer *project_symbols,
                                            int is_sync,
                                            int nesting_depth,
                                            JZBuffer *paths,
                                            JZDiagnosticList *diagnostics)
{
    if (!stmt || !paths) return;

    int child_depth = nesting_depth + 1;

    JZASTNode **body_children = NULL;
    size_t body_count = 0;

    if (stmt->type == JZ_AST_STMT_IF || stmt->type == JZ_AST_STMT_ELIF) {
        /* Skip child[0] (the condition); body is children[1..N] */
        if (stmt->child_count > 1) {
            body_children = &stmt->children[1];
            body_count = stmt->child_count - 1;
        }
    } else if (stmt->type == JZ_AST_STMT_ELSE) {
        body_children = stmt->children;
        body_count = stmt->child_count;
    }

    if (body_children && body_count > 0) {
        JZASTNode fake_block;
        memset(&fake_block, 0, sizeof(fake_block));
        fake_block.type = JZ_AST_BLOCK;
        fake_block.children = body_children;
        fake_block.child_count = body_count;
        sem_excl_analyze_block(&fake_block, scope, project_symbols, is_sync, child_depth, paths, diagnostics);
    }
}

/* -------------------------------------------------------------------------
 *  Set-union approach for IF/SELECT branching.
 *
 *  Instead of enumerating all execution paths (which grows exponentially),
 *  we process each branch independently from a cloned base set and then
 *  MERGE all branch results into a single set (union of assignments).
 *
 *  Correctness: if variable X appears in the merged set, at least one branch
 *  assigned it.  Any subsequent assignment to X on this merged path represents
 *  a real double-assignment on the execution path through that branch.
 *  Conflicts *within* a branch are caught during branch processing.
 * -------------------------------------------------------------------------
 */

/* Helper: check if a decl appears in a set of new assignment records */
static int sem_excl_decl_in_records(const JZAssignRecord *recs,
                                     size_t count,
                                     const JZASTNode *decl)
{
    for (size_t i = 0; i < count; ++i) {
        if (recs[i].key.decl == decl) return 1;
    }
    return 0;
}

static void sem_excl_analyze_if_chain(JZASTNode *block,
                                      size_t start_index,
                                      size_t end_index,
                                      const JZModuleScope *scope,
                                      const JZBuffer *project_symbols,
                                      int is_sync,
                                      int nesting_depth,
                                      JZBuffer *paths,
                                      JZDiagnosticList *diagnostics)
{
    if (!block || !scope || !paths) return;
    if (end_index <= start_index) return;

    size_t branch_count = end_index - start_index;
    int has_else = 0;
    JZASTNode *last = block->children[end_index - 1];
    if (last && last->type == JZ_AST_STMT_ELSE) {
        has_else = 1;
    }

    size_t path_count = paths->len / sizeof(JZPathState);
    JZPathState *base_arr = (JZPathState *)paths->data;

    for (size_t pi = 0; pi < path_count; ++pi) {
        JZPathState *base = &base_arr[pi];
        size_t base_rec_count = base->assigns.len / sizeof(JZAssignRecord);

        /* Collect new assignments from each branch separately so we can
           compute which decls appear in ALL branches (for partial tracking). */
        JZBuffer *branch_new = (JZBuffer *)calloc(branch_count, sizeof(JZBuffer));
        if (!branch_new) continue;

        for (size_t bi = 0; bi < branch_count; ++bi) {
            JZASTNode *stmt = block->children[start_index + bi];

            JZPathState clone;
            if (sem_excl_clone_path_state(base, &clone) != 0) {
                continue;
            }

            JZBuffer branch_paths = (JZBuffer){0};
            (void)jz_buf_append(&branch_paths, &clone, sizeof(JZPathState));
            sem_excl_analyze_if_branch_body(stmt, scope, project_symbols, is_sync, nesting_depth, &branch_paths, diagnostics);

            /* Extract new records (beyond base) from this branch */
            size_t bp_count = branch_paths.len / sizeof(JZPathState);
            JZPathState *branch_result = (JZPathState *)branch_paths.data;
            for (size_t bpi = 0; bpi < bp_count; ++bpi) {
                JZPathState *br = &branch_result[bpi];
                size_t br_rec_count = br->assigns.len / sizeof(JZAssignRecord);
                if (br_rec_count > base_rec_count) {
                    JZAssignRecord *recs = (JZAssignRecord *)br->assigns.data;
                    size_t new_count = br_rec_count - base_rec_count;
                    (void)jz_buf_append(&branch_new[bi],
                                        &recs[base_rec_count],
                                        new_count * sizeof(JZAssignRecord));
                }
                jz_buf_free(&br->assigns);
            }
            jz_buf_free(&branch_paths);
        }

        /* Merge all branch assignments into the base path, marking partial
           status based on whether the decl appears in ALL branches.
           Without ELSE, all branch assignments are partial (there's a path
           that skips the IF entirely). */
        for (size_t bi = 0; bi < branch_count; ++bi) {
            size_t rec_count = branch_new[bi].len / sizeof(JZAssignRecord);
            JZAssignRecord *recs = (JZAssignRecord *)branch_new[bi].data;

            for (size_t ri = 0; ri < rec_count; ++ri) {
                JZAssignRecord *rec = &recs[ri];

                if (!has_else) {
                    rec->partial = 1;
                } else {
                    /* With ELSE, check if this decl appears in every branch */
                    int in_all = 1;
                    for (size_t bj = 0; bj < branch_count; ++bj) {
                        if (bj == bi) continue;
                        size_t other_count = branch_new[bj].len / sizeof(JZAssignRecord);
                        JZAssignRecord *other_recs = (JZAssignRecord *)branch_new[bj].data;
                        if (!sem_excl_decl_in_records(other_recs, other_count, rec->key.decl)) {
                            in_all = 0;
                            break;
                        }
                    }
                    /* Only upgrade to partial=1 when the decl is missing in
                     * a sibling branch. Preserve any partial=1 flag already
                     * set by recursive inner-branch analysis (e.g. a nested
                     * IF without ELSE inside this branch body) — the outer
                     * having full coverage does not negate an inner
                     * uncovered path. */
                    if (!in_all) {
                        rec->partial = 1;
                    }
                }
            }

            if (branch_new[bi].len > 0) {
                (void)jz_buf_append(&base->assigns,
                                    branch_new[bi].data,
                                    branch_new[bi].len);
            }
            jz_buf_free(&branch_new[bi]);
        }
        free(branch_new);
    }
}

static void sem_excl_analyze_select(JZASTNode *select_stmt,
                                    const JZModuleScope *scope,
                                    const JZBuffer *project_symbols,
                                    int is_sync,
                                    int nesting_depth,
                                    JZBuffer *paths,
                                    JZDiagnosticList *diagnostics)
{
    if (!select_stmt || !paths) return;
    if (select_stmt->child_count < 2) return;

    size_t path_count = paths->len / sizeof(JZPathState);
    JZPathState *base_arr = (JZPathState *)paths->data;

    size_t case_start = 1;
    size_t case_end = select_stmt->child_count;

    int has_default = 0;
    for (size_t ci = case_start; ci < case_end; ++ci) {
        JZASTNode *case_node = select_stmt->children[ci];
        if (case_node && case_node->type == JZ_AST_STMT_DEFAULT) {
            has_default = 1;
            break;
        }
    }
    int has_exhaustive_coverage = (!has_default &&
        sem_dead_select_cases_exhaustive(select_stmt, scope, project_symbols));

    /* Count actual case branches (excluding empty ones) */
    size_t actual_branch_count = 0;
    for (size_t ci = case_start; ci < case_end; ++ci) {
        JZASTNode *case_node = select_stmt->children[ci];
        if (!case_node) continue;
        size_t body_start = (case_node->type == JZ_AST_STMT_CASE) ? 1u : 0u;
        if (case_node->child_count > body_start) actual_branch_count++;
    }

    for (size_t pi = 0; pi < path_count; ++pi) {
        JZPathState *base = &base_arr[pi];
        size_t base_rec_count = base->assigns.len / sizeof(JZAssignRecord);

        /* Collect new assignments per case branch */
        JZBuffer *branch_new = (JZBuffer *)calloc(actual_branch_count, sizeof(JZBuffer));
        if (!branch_new) continue;

        size_t bi = 0;
        for (size_t ci = case_start; ci < case_end; ++ci) {
            JZASTNode *case_node = select_stmt->children[ci];
            if (!case_node) continue;

            size_t body_start = (case_node->type == JZ_AST_STMT_CASE) ? 1u : 0u;
            if (case_node->child_count <= body_start) continue;

            JZPathState clone;
            if (sem_excl_clone_path_state(base, &clone) != 0) {
                bi++;
                continue;
            }
            JZBuffer branch_paths = (JZBuffer){0};
            (void)jz_buf_append(&branch_paths, &clone, sizeof(JZPathState));

            for (size_t bj = body_start; bj < case_node->child_count; ++bj) {
                JZASTNode *body = case_node->children[bj];
                sem_excl_analyze_stmt(body, scope, project_symbols, is_sync, nesting_depth + 1, &branch_paths, diagnostics);
            }

            size_t bp_count = branch_paths.len / sizeof(JZPathState);
            JZPathState *branch_result = (JZPathState *)branch_paths.data;
            for (size_t bpi = 0; bpi < bp_count; ++bpi) {
                JZPathState *br = &branch_result[bpi];
                size_t br_rec_count = br->assigns.len / sizeof(JZAssignRecord);
                if (br_rec_count > base_rec_count) {
                    JZAssignRecord *recs = (JZAssignRecord *)br->assigns.data;
                    size_t new_count = br_rec_count - base_rec_count;
                    (void)jz_buf_append(&branch_new[bi],
                                        &recs[base_rec_count],
                                        new_count * sizeof(JZAssignRecord));
                }
                jz_buf_free(&br->assigns);
            }
            jz_buf_free(&branch_paths);
            bi++;
        }

        /* Merge, marking partial status */
        for (bi = 0; bi < actual_branch_count; ++bi) {
            size_t rec_count = branch_new[bi].len / sizeof(JZAssignRecord);
            JZAssignRecord *recs = (JZAssignRecord *)branch_new[bi].data;

            for (size_t ri = 0; ri < rec_count; ++ri) {
                JZAssignRecord *rec = &recs[ri];

                if (!has_default && !has_exhaustive_coverage) {
                    rec->partial = 1;
                } else {
                    int in_all = 1;
                    for (size_t bj = 0; bj < actual_branch_count; ++bj) {
                        if (bj == bi) continue;
                        size_t other_count = branch_new[bj].len / sizeof(JZAssignRecord);
                        JZAssignRecord *other_recs = (JZAssignRecord *)branch_new[bj].data;
                        if (!sem_excl_decl_in_records(other_recs, other_count, rec->key.decl)) {
                            in_all = 0;
                            break;
                        }
                    }
                    /* Preserve inner-branch partial flags (see matching
                     * comment in sem_excl_analyze_if_chain). */
                    if (!in_all) {
                        rec->partial = 1;
                    }
                }
            }

            if (branch_new[bi].len > 0) {
                (void)jz_buf_append(&base->assigns,
                                    branch_new[bi].data,
                                    branch_new[bi].len);
            }
            jz_buf_free(&branch_new[bi]);
        }
        free(branch_new);
    }
}

static void sem_excl_analyze_stmt(JZASTNode *stmt,
                                  const JZModuleScope *scope,
                                  const JZBuffer *project_symbols,
                                  int is_sync,
                                  int nesting_depth,
                                  JZBuffer *paths,
                                  JZDiagnosticList *diagnostics)
{
    if (!stmt || !scope || !paths) return;

    switch (stmt->type) {
    case JZ_AST_STMT_ASSIGN: {
        if (stmt->child_count < 2) break;
        JZASTNode *lhs = stmt->children[0];
        JZASTNode *rhs = stmt->children[1];

        const char *op = stmt->block_kind ? stmt->block_kind : "";
        int is_drive   = (strncmp(op, "DRIVE", 5) == 0);

        /* Choose the real assignment target based on directional semantics:
         *   - DRIVE*:  source => sink   (RHS is the driven net)
         *   - RECEIVE*: sink <= source  (LHS is the driven net)
         *   - plain '=' or anything else: LHS is the driven net.
         */
        JZASTNode *target_expr = lhs;
        if (is_drive) {
            target_expr = rhs;
        }

        JZBuffer targets = (JZBuffer){0};
        sem_excl_collect_targets_from_lhs(target_expr, scope, project_symbols, is_sync, nesting_depth > 0, &targets);

        size_t target_count = targets.len / sizeof(JZAssignTargetEntry);
        JZAssignTargetEntry *entries = (JZAssignTargetEntry *)targets.data;
        int can_drive_z = sem_flow_expr_contains_z_literal(rhs);
        for (size_t ti = 0; ti < target_count; ++ti) {
            entries[ti].can_drive_z = can_drive_z ||
                sem_flow_node_has_z_assignment_to_decl(scope->node, entries[ti].decl);
        }

        size_t path_count = paths->len / sizeof(JZPathState);
        JZPathState *path_arr = (JZPathState *)paths->data;
        for (size_t pi = 0; pi < path_count; ++pi) {
            sem_excl_record_assignment_in_path(&path_arr[pi],
                                               entries,
                                               target_count,
                                               is_sync,
                                               stmt->loc,
                                               diagnostics);
        }

        jz_buf_free(&targets);
        break;
    }

    case JZ_AST_STMT_IF: {
        JZASTNode fake_block;
        memset(&fake_block, 0, sizeof(fake_block));
        fake_block.type = JZ_AST_BLOCK;
        fake_block.children = &stmt;
        fake_block.child_count = 1;
        sem_excl_analyze_if_chain(&fake_block, 0, 1, scope, project_symbols, is_sync, nesting_depth, paths, diagnostics);
        break;
    }

    case JZ_AST_STMT_ELIF:
    case JZ_AST_STMT_ELSE:
        break;

    case JZ_AST_STMT_SELECT:
        sem_excl_analyze_select(stmt, scope, project_symbols, is_sync, nesting_depth, paths, diagnostics);
        break;

    case JZ_AST_BLOCK:
        for (size_t i = 0; i < stmt->child_count; ++i) {
            JZASTNode *child = stmt->children[i];
            if (!child) continue;
            sem_excl_analyze_stmt(child, scope, project_symbols, is_sync, nesting_depth + 1, paths, diagnostics);
        }
        break;

    default:
        break;
    }
}

static void sem_excl_analyze_block(JZASTNode *block,
                                   const JZModuleScope *scope,
                                   const JZBuffer *project_symbols,
                                   int is_sync,
                                   int nesting_depth,
                                   JZBuffer *paths,
                                   JZDiagnosticList *diagnostics)
{
    if (!block || !scope || !paths) return;

    for (size_t i = 0; i < block->child_count; ++i) {
        JZASTNode *stmt = block->children[i];
        if (!stmt) continue;

        if (stmt->type == JZ_AST_STMT_IF) {
            size_t start = i;
            size_t end = i + 1;
            while (end < block->child_count) {
                JZASTNode *next = block->children[end];
                if (!next) {
                    end++;
                    continue;
                }
                if (next->type == JZ_AST_STMT_ELIF || next->type == JZ_AST_STMT_ELSE) {
                    end++;
                    if (next->type == JZ_AST_STMT_ELSE) {
                        break;
                    }
                } else {
                    break;
                }
            }
            sem_excl_analyze_if_chain(block, start, end, scope, project_symbols, is_sync, nesting_depth, paths, diagnostics);
            i = end - 1;
        } else {
            sem_excl_analyze_stmt(stmt, scope, project_symbols, is_sync, nesting_depth, paths, diagnostics);
        }
    }
}

static void sem_excl_check_async_undefined_paths(const JZModuleScope *scope,
                                                 JZASTNode *block,
                                                 JZBuffer *paths,
                                                 JZDiagnosticList *diagnostics)
{
    if (!scope || !scope->node || !block || !paths) return;

    size_t path_count = paths->len / sizeof(JZPathState);
    if (path_count == 0) return;

    JZBuffer decls = (JZBuffer){0};

    JZPathState *path_arr = (JZPathState *)paths->data;
    for (size_t pi = 0; pi < path_count; ++pi) {
        JZAssignRecord *recs = (JZAssignRecord *)path_arr[pi].assigns.data;
        size_t rec_count = path_arr[pi].assigns.len / sizeof(JZAssignRecord);
        for (size_t ri = 0; ri < rec_count; ++ri) {
            JZAssignRecord *r = &recs[ri];
            if (!r->key.decl || r->key.is_register) continue;

            int seen = 0;
            JZASTNode **decl_arr = (JZASTNode **)decls.data;
            size_t decl_count = decls.len / sizeof(JZASTNode *);
            for (size_t di = 0; di < decl_count; ++di) {
                if (decl_arr[di] == r->key.decl) {
                    seen = 1;
                    break;
                }
            }
            if (!seen) {
                (void)jz_buf_append(&decls, &r->key.decl, sizeof(JZASTNode *));
            }
        }
    }

    JZASTNode **decl_arr = (JZASTNode **)decls.data;
    size_t decl_count = decls.len / sizeof(JZASTNode *);

    for (size_t di = 0; di < decl_count; ++di) {
        JZASTNode *decl = decl_arr[di];
        if (!decl) continue;

        /* Enforce ASYNC_UNDEFINED_PATH_NO_DRIVER for:
         * 1. Nets whose value is read somewhere in this ASYNCHRONOUS block
         * 2. OUT and INOUT ports (which are always observable externally)
         * Nets that are partially assigned but never inspected are allowed
         * (other rules such as WARN_UNUSED_REGISTER/NET_DANGLING_UNUSED may
         * still apply).
         */
        const char *name = decl->name;
        if (!name) continue;

        /* OUT and INOUT ports are always observable externally, so they need
         * to be driven in all paths regardless of whether they're read within
         * this block.
         */
        int is_out_or_inout_port = 0;
        if (decl->block_kind &&
            (strcmp(decl->block_kind, "OUT") == 0 ||
             strcmp(decl->block_kind, "INOUT") == 0)) {
            is_out_or_inout_port = 1;
        }

        if (!is_out_or_inout_port && !sem_block_reads_name(block, name)) {
            continue;
        }

        int assigned_in_all_paths = 1;
        for (size_t pi = 0; pi < path_count && assigned_in_all_paths; ++pi) {
            JZAssignRecord *recs = (JZAssignRecord *)path_arr[pi].assigns.data;
            size_t rec_count = path_arr[pi].assigns.len / sizeof(JZAssignRecord);
            int found = 0;
            for (size_t ri = 0; ri < rec_count; ++ri) {
                JZAssignRecord *r = &recs[ri];
                if (r->key.decl == decl && !r->key.is_register && !r->partial) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                assigned_in_all_paths = 0;
                break;
            }
        }

        if (!assigned_in_all_paths) {
            sem_report_rule(diagnostics,
                            decl->loc,
                            "ASYNC_UNDEFINED_PATH_NO_DRIVER",
                            "ASYNCHRONOUS path leaves net without any driver (missing ELSE/DEFAULT or unconditional assignment)");
        }
    }

    jz_buf_free(&decls);
}

void sem_check_exclusive_assignments(JZASTNode *root,
                                     JZBuffer *module_scopes,
                                     const JZBuffer *project_symbols,
                                     JZDiagnosticList *diagnostics)
{
    if (!root || root->type != JZ_AST_PROJECT || !module_scopes) return;
    (void)root;

    size_t scope_count = module_scopes->len / sizeof(JZModuleScope);
    JZModuleScope *scopes = (JZModuleScope *)module_scopes->data;

    for (size_t i = 0; i < scope_count; ++i) {
        JZModuleScope *scope = &scopes[i];
        if (!scope->node) continue;

        JZASTNode *mod = scope->node;
        for (size_t ci = 0; ci < mod->child_count; ++ci) {
            JZASTNode *child = mod->children[ci];
            if (!child || child->type != JZ_AST_BLOCK || !child->block_kind) continue;

            int is_async = (strcmp(child->block_kind, "ASYNCHRONOUS") == 0);
            int is_sync  = (strcmp(child->block_kind, "SYNCHRONOUS") == 0);
            if (!is_async && !is_sync) continue;

            JZBuffer paths = (JZBuffer){0};
            JZPathState initial = (JZPathState){0};
            (void)jz_buf_append(&paths, &initial, sizeof(initial));

            sem_excl_analyze_block(child, scope, project_symbols, is_sync, 0, &paths, diagnostics);

            if (is_async) {
                sem_excl_check_async_undefined_paths(scope, child, &paths, diagnostics);
            }

            sem_excl_free_paths(&paths);
        }
    }
}


/* -------------------------------------------------------------------------
 *  Dead code detection (WARN_DEAD_CODE_UNREACHABLE, MEM_WARN_DEAD_CODE_ACCESS)
 * -------------------------------------------------------------------------
 */

static int sem_expr_is_const_zero_literal(const JZASTNode *expr,
                                          int *out_known)
{
    if (out_known) *out_known = 0;
    if (!expr || expr->type != JZ_AST_EXPR_LITERAL || !expr->text) {
        return 0;
    }
    return sem_literal_is_const_zero(expr->text, out_known);
}

static int sem_expr_is_const_nonzero_literal(const JZASTNode *expr,
                                             int *out_known)
{
    int known = 0;
    int is_zero = sem_expr_is_const_zero_literal(expr, &known);
    if (out_known) *out_known = known;
    if (!known) return 0;
    return is_zero ? 0 : 1;
}

static void sem_dead_scan_node_for_mem_access(JZASTNode *node,
                                              const JZModuleScope *scope,
                                              JZDiagnosticList *diagnostics)
{
    if (!node || !scope || !diagnostics) return;

    if (node->type == JZ_AST_EXPR_SLICE) {
        JZMemPortRef ref;
        memset(&ref, 0, sizeof(ref));
        if (sem_match_mem_port_slice(node, scope, NULL, &ref) && ref.port) {
            sem_report_rule(diagnostics,
                            node->loc,
                            "MEM_WARN_DEAD_CODE_ACCESS",
                            "memory access appears only in unreachable code");
        }
    }
    if (node->type == JZ_AST_EXPR_QUALIFIED_IDENTIFIER) {
        JZMemPortRef ref;
        memset(&ref, 0, sizeof(ref));
        if (sem_match_mem_port_qualified_ident(node, scope, NULL, &ref) &&
            ref.port && (ref.field == MEM_PORT_FIELD_ADDR ||
                         ref.field == MEM_PORT_FIELD_DATA ||
                         ref.field == MEM_PORT_FIELD_WDATA)) {
            sem_report_rule(diagnostics,
                            node->loc,
                            "MEM_WARN_DEAD_CODE_ACCESS",
                            "memory access appears only in unreachable code");
        }
    }

    for (size_t i = 0; i < node->child_count; ++i) {
        sem_dead_scan_node_for_mem_access(node->children[i], scope, diagnostics);
    }
}

static void sem_dead_mark_unreachable_branch_body(JZASTNode *stmt,
                                                   const JZModuleScope *scope,
                                                   JZDiagnosticList *diagnostics)
{
    if (!stmt || !scope || !diagnostics) return;

    int warned = 0;

    JZBuffer stack = (JZBuffer){0};
    (void)jz_buf_append(&stack, &stmt, sizeof(JZASTNode *));

    while (stack.len > 0) {
        size_t count = stack.len / sizeof(JZASTNode *);
        JZASTNode **arr = (JZASTNode **)stack.data;
        JZASTNode *node = arr[count - 1];
        stack.len -= sizeof(JZASTNode *);

        if (!node) continue;

        if (!warned && (node->type == JZ_AST_STMT_ASSIGN ||
                        node->type == JZ_AST_STMT_IF ||
                        node->type == JZ_AST_STMT_ELIF ||
                        node->type == JZ_AST_STMT_ELSE ||
                        node->type == JZ_AST_STMT_SELECT ||
                        node->type == JZ_AST_STMT_CASE ||
                        node->type == JZ_AST_STMT_DEFAULT)) {
            sem_report_rule(diagnostics,
                            node->loc,
                            "WARN_DEAD_CODE_UNREACHABLE",
                            "statement is statically unreachable (dead code)");
            warned = 1;
        }

        sem_dead_scan_node_for_mem_access(node, scope, diagnostics);

        for (size_t i = 0; i < node->child_count; ++i) {
            JZASTNode *child = node->children[i];
            if (child) {
                (void)jz_buf_append(&stack, &child, sizeof(JZASTNode *));
            }
        }
    }

    jz_buf_free(&stack);
}

static void sem_dead_check_if_chain(JZASTNode *block,
                                    size_t start_index,
                                    size_t end_index,
                                    const JZModuleScope *scope,
                                    JZDiagnosticList *diagnostics)
{
    if (!block || !scope || !diagnostics) return;
    if (end_index <= start_index) return;

    size_t branch_count = end_index - start_index;
    int always_true[64];
    int always_false[64];
    int prior_true[64];
    memset(always_true, 0, sizeof(always_true));
    memset(always_false, 0, sizeof(always_false));
    memset(prior_true, 0, sizeof(prior_true));

    if (branch_count > 64) {
        branch_count = 64;
    }

    for (size_t bi = 0; bi < branch_count; ++bi) {
        JZASTNode *stmt = block->children[start_index + bi];
        if (!stmt) continue;
        if (stmt->type != JZ_AST_STMT_IF && stmt->type != JZ_AST_STMT_ELIF) {
            continue;
        }
        if (stmt->child_count == 0) continue;
        JZASTNode *cond = stmt->children[0];
        int known = 0;
        if (sem_expr_is_const_zero_literal(cond, &known) && known) {
            always_false[bi] = 1;
        } else if (sem_expr_is_const_nonzero_literal(cond, &known) && known) {
            always_true[bi] = 1;
        }
    }

    for (size_t bi = 0; bi < branch_count; ++bi) {
        prior_true[bi] = 0;
        for (size_t pj = 0; pj < bi; ++pj) {
            if (always_true[pj]) {
                prior_true[bi] = 1;
                break;
            }
        }
    }

    for (size_t bi = 0; bi < branch_count; ++bi) {
        JZASTNode *stmt = block->children[start_index + bi];
        if (!stmt) continue;

        int unreachable = 0;
        if (stmt->type == JZ_AST_STMT_IF || stmt->type == JZ_AST_STMT_ELIF) {
            if (always_false[bi] || prior_true[bi]) {
                unreachable = 1;
            }
        } else if (stmt->type == JZ_AST_STMT_ELSE) {
            if (prior_true[bi]) {
                unreachable = 1;
            }
        }

        if (!unreachable) {
            continue;
        }

        size_t body_start = (stmt->type == JZ_AST_STMT_ELSE) ? 0u : 1u;
        for (size_t j = body_start; j < stmt->child_count; ++j) {
            JZASTNode *body = stmt->children[j];
            if (!body) continue;
            sem_dead_mark_unreachable_branch_body(body, scope, diagnostics);
        }
    }
}

static int sem_dead_select_cases_exhaustive(JZASTNode *select_stmt,
                                            const JZModuleScope *scope,
                                            const JZBuffer *project_symbols)
{
    if (!select_stmt || select_stmt->type != JZ_AST_STMT_SELECT ||
        select_stmt->child_count < 2 || !scope) {
        return 0;
    }

    JZASTNode *selector = select_stmt->children[0];
    if (!selector) return 0;

    JZBitvecType sel_t;
    sel_t.width = 0;
    sel_t.is_signed = 0;
    infer_expr_type(selector, scope, project_symbols, NULL, &sel_t);
    if (sel_t.width == 0 || sel_t.width > 20) {
        return 0;
    }

    size_t total = (size_t)1u << sel_t.width;
    unsigned char *seen = (unsigned char *)calloc(total, sizeof(unsigned char));
    if (!seen) return 0;

    size_t covered = 0;
    for (size_t i = 1; i < select_stmt->child_count; ++i) {
        JZASTNode *case_node = select_stmt->children[i];
        if (!case_node || case_node->type != JZ_AST_STMT_CASE ||
            case_node->child_count == 0) {
            continue;
        }

        JZASTNode *val = case_node->children[0];
        if (!val || val->type != JZ_AST_EXPR_LITERAL || !val->text) {
            continue;
        }

        unsigned value = 0;
        if (!parse_literal_unsigned_value(val->text, &value)) {
            continue;
        }
        if ((size_t)value >= total || seen[value]) {
            continue;
        }

        seen[value] = 1;
        covered++;
        if (covered == total) {
            free(seen);
            return 1;
        }
    }

    free(seen);
    return 0;
}

static void sem_dead_check_select(JZASTNode *select_stmt,
                                  const JZModuleScope *scope,
                                  const JZBuffer *project_symbols,
                                  JZDiagnosticList *diagnostics)
{
    if (!select_stmt || !scope || !diagnostics) return;
    if (select_stmt->type != JZ_AST_STMT_SELECT) return;
    if (!sem_dead_select_cases_exhaustive(select_stmt, scope, project_symbols)) {
        return;
    }

    for (size_t i = 1; i < select_stmt->child_count; ++i) {
        JZASTNode *branch = select_stmt->children[i];
        if (!branch || branch->type != JZ_AST_STMT_DEFAULT) continue;

        for (size_t j = 0; j < branch->child_count; ++j) {
            JZASTNode *body = branch->children[j];
            if (!body) continue;
            sem_dead_mark_unreachable_branch_body(body, scope, diagnostics);
        }
        return;
    }
}

static void sem_check_dead_code_for_block(JZASTNode *block,
                                          const JZModuleScope *scope,
                                          const JZBuffer *project_symbols,
                                          JZDiagnosticList *diagnostics)
{
    if (!block || !scope) return;

    for (size_t i = 0; i < block->child_count; ++i) {
        JZASTNode *stmt = block->children[i];
        if (!stmt) continue;

        if (stmt->type == JZ_AST_STMT_IF) {
            size_t start = i;
            size_t end = i + 1;
            while (end < block->child_count) {
                JZASTNode *next = block->children[end];
                if (!next) {
                    end++;
                    continue;
                }
                if (next->type == JZ_AST_STMT_ELIF || next->type == JZ_AST_STMT_ELSE) {
                    end++;
                    if (next->type == JZ_AST_STMT_ELSE) {
                        break;
                    }
                } else {
                    break;
                }
            }

            sem_dead_check_if_chain(block, start, end, scope, diagnostics);
            i = end - 1;
        } else if (stmt->type == JZ_AST_STMT_SELECT) {
            sem_dead_check_select(stmt, scope, project_symbols, diagnostics);
        }
    }
}

void sem_check_dead_code(JZASTNode *root,
                         JZBuffer *module_scopes,
                         const JZBuffer *project_symbols,
                         JZDiagnosticList *diagnostics)
{
    if (!root || root->type != JZ_AST_PROJECT || !module_scopes) return;
    size_t scope_count = module_scopes->len / sizeof(JZModuleScope);
    JZModuleScope *scopes = (JZModuleScope *)module_scopes->data;

    for (size_t i = 0; i < scope_count; ++i) {
        JZModuleScope *scope = &scopes[i];
        if (!scope->node) continue;
        JZASTNode *mod = scope->node;
        for (size_t ci = 0; ci < mod->child_count; ++ci) {
            JZASTNode *child = mod->children[ci];
            if (!child || child->type != JZ_AST_BLOCK || !child->block_kind) continue;
            int is_async = (strcmp(child->block_kind, "ASYNCHRONOUS") == 0);
            int is_sync  = (strcmp(child->block_kind, "SYNCHRONOUS") == 0);
            (void)is_async;
            (void)is_sync;
            if (!is_async && !is_sync) continue;
            sem_check_dead_code_for_block(child, scope, project_symbols, diagnostics);
        }
    }
}
