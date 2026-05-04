/**
 * @file emit_process.c
 * @brief Emits RTLIL processes for asynchronous and clocked logic.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#include "rtlil_internal.h"
#include "ir.h"
#include "util.h"

/* Reuse Verilog backend helpers. */
#include "backend/verilog-2005/verilog_internal.h"

/* -------------------------------------------------------------------------
 * Process context: maps signal IDs to intermediate wire names
 *
 * When emitting a process, each assigned signal gets an intermediate wire
 * (e.g., $0\pbus_ADDR[7:0]). The process body assigns to intermediates,
 * and sync blocks use `update` to copy intermediates to real signals.
 * -------------------------------------------------------------------------
 */

/**
 * @struct ProcessIntermediate
 * @brief Maps one assigned signal to its process-local temporary wire.
 */
typedef struct {
    int signal_id;      /**< IR signal identifier for the assigned signal. */
    char name[128];     /**< Generated RTLIL sigspec for the temporary wire. */
} ProcessIntermediate;

/**
 * @struct ProcessContext
 * @brief Tracks process-local temporary wires by signal ID.
 */
typedef struct {
    ProcessIntermediate *entries;  /**< Dynamic array of intermediate mappings. */
    int count;                     /**< Number of initialized mappings. */
    int capacity;                  /**< Allocated entry capacity. */
} ProcessContext;

/**
 * @brief Initialize an empty process context.
 * @param ctx Context to initialize.
 */
static void pctx_init(ProcessContext *ctx);

/**
 * @brief Release storage held by a process context.
 * @param ctx Context to clear.
 */
static void pctx_free(ProcessContext *ctx);

/**
 * @brief Look up the temporary-wire sigspec for an assigned signal.
 * @param ctx Context to query.
 * @param signal_id IR signal identifier to look up.
 * @return Temporary sigspec string, or `NULL` if the signal is not tracked.
 */
static const char *pctx_find(const ProcessContext *ctx, int signal_id);

/**
 * @brief Record a new temporary-wire mapping for a signal.
 * @param ctx Context to extend.
 * @param signal_id IR signal identifier to map.
 * @param name Generated temporary sigspec to store.
 */
static void pctx_add(ProcessContext *ctx, int signal_id, const char *name);

static void pctx_init(ProcessContext *ctx) {
    ctx->entries = NULL;
    ctx->count = 0;
    ctx->capacity = 0;
}

static void pctx_free(ProcessContext *ctx) {
    if (ctx->entries) free(ctx->entries);
    ctx->entries = NULL;
    ctx->count = ctx->capacity = 0;
}

static const char *pctx_find(const ProcessContext *ctx, int signal_id) {
    for (int i = 0; i < ctx->count; ++i) {
        if (ctx->entries[i].signal_id == signal_id)
            return ctx->entries[i].name;
    }
    return NULL;
}

static void pctx_add(ProcessContext *ctx, int signal_id, const char *name) {
    if (pctx_find(ctx, signal_id)) return;
    if (ctx->count >= ctx->capacity) {
        int new_cap = 0;
        size_t new_bytes = 0;
        ProcessIntermediate *new_entries = NULL;
        if (ctx->capacity == 0) {
            new_cap = 16;
        } else if (ctx->capacity > INT_MAX / 2) {
            return;
        } else {
            new_cap = ctx->capacity * 2;
        }
        if (new_cap <= ctx->capacity ||
            jz_size_mul_checked((size_t)new_cap, sizeof(ProcessIntermediate), &new_bytes) != 0) {
            return;
        }
        new_entries = realloc(ctx->entries, new_bytes);
        if (!new_entries) return;
        ctx->entries = new_entries;
        ctx->capacity = new_cap;
    }
    ctx->entries[ctx->count].signal_id = signal_id;
    snprintf(ctx->entries[ctx->count].name,
             sizeof(ctx->entries[0].name), "%s", name);
    ctx->count++;
}

/**
 * @brief Emit one statement subtree inside a process.
 * @param cell_out Stream for module-scope helper wires and cells.
 * @param proc_out Stream for process-body statements.
 * @param mod Module that owns the statement.
 * @param stmt Statement subtree to emit.
 * @param indent Indentation depth for process-body output.
 * @param pctx Process context for temporary assignment wires.
 */
static void emit_process_stmt(FILE *cell_out, FILE *proc_out,
                              const IR_Module *mod, const IR_Stmt *stmt,
                              int indent, const ProcessContext *pctx);

/**
 * @brief Emit a block statement inside a process.
 * @param cell_out Stream for module-scope helper wires and cells.
 * @param proc_out Stream for process-body statements.
 * @param mod Module that owns the block.
 * @param block Block statement to emit.
 * @param indent Indentation depth for process-body output.
 * @param pctx Process context for temporary assignment wires.
 */
static void emit_process_block(FILE *cell_out, FILE *proc_out,
                               const IR_Module *mod, const IR_Stmt *block,
                               int indent, const ProcessContext *pctx);

/**
 * @struct SignalIdList
 * @brief Stores the set of signal IDs assigned within a process.
 */
typedef struct {
    int *ids;         /**< Dynamic array of collected signal IDs. */
    int count;        /**< Number of collected IDs. */
    int capacity;     /**< Allocated entry capacity. */
} SignalIdList;

/**
 * @brief Initialize an empty signal-ID list.
 * @param list List to initialize.
 */
static void siglist_init(SignalIdList *list);

/**
 * @brief Release storage held by a signal-ID list.
 * @param list List to clear.
 */
static void siglist_free(SignalIdList *list);

/**
 * @brief Add a signal ID to a set-like list if it is not already present.
 * @param list List to extend.
 * @param id IR signal identifier to record.
 */
static void siglist_add(SignalIdList *list, int id);

/**
 * @brief Collect assigned signal IDs from a statement subtree.
 * @param stmt Statement subtree to inspect.
 * @param list Destination set of assigned signal IDs.
 */
static void collect_assigned_signals(const IR_Stmt *stmt, SignalIdList *list);

/**
 * @brief Emit default assignments for tracked process temporaries.
 * @param proc_out Destination stream for process-body statements.
 * @param mod Module that owns the assigned signals.
 * @param assigned Set of assigned signal IDs.
 * @param pctx Process context that maps signals to temporaries.
 * @param indent Indentation depth for emitted statements.
 */
static void emit_process_defaults(FILE *proc_out, const IR_Module *mod,
                                  const SignalIdList *assigned,
                                  const ProcessContext *pctx, int indent);

/**
 * @brief Build process temporaries and emit their module-scope wires.
 * @param out Destination RTLIL stream for wire declarations.
 * @param mod Module that owns the signals.
 * @param assigned Set of assigned signal IDs.
 * @param pctx Context to populate with generated temporaries.
 */
static void build_process_context(FILE *out, const IR_Module *mod,
                                  const SignalIdList *assigned,
                                  ProcessContext *pctx);

/**
 * @brief Copy a temporary file stream into the final RTLIL output.
 * @param tmp Temporary stream to rewind and drain.
 * @param out Destination stream that receives the copied bytes.
 */
static void flush_tmpfile_to(FILE *tmp, FILE *out);

/**
 * @brief Emit the left-hand-side sigspec for a process assignment.
 * @param proc_out Destination process-body stream.
 * @param mod Module that owns the assignment.
 * @param a Assignment being emitted.
 * @param sig Resolved left-hand-side signal.
 * @param pctx Process context for temporary assignment wires.
 */
static void emit_lhs_sigspec(FILE *proc_out, const IR_Module *mod,
                             const IR_Assignment *a, const IR_Signal *sig,
                             const ProcessContext *pctx);

/**
 * @brief Build the right-hand-side sigspec for a process assignment.
 * @param cell_out Stream for helper cells and wires.
 * @param rhs_buf Destination buffer.
 * @param rhs_buf_size Size of `rhs_buf` in bytes.
 * @param mod Module that owns the assignment.
 * @param a Assignment being emitted.
 * @param lhs_sig Resolved left-hand-side signal used for width decisions.
 */
static void emit_rhs_sigspec(FILE *cell_out, char *rhs_buf, int rhs_buf_size,
                             const IR_Module *mod, const IR_Assignment *a,
                             const IR_Signal *lhs_sig);

/**
 * @brief Estimate the effective emitted width of an expression sigspec.
 * @param expr Expression to inspect.
 * @param mod Module used to resolve signal references.
 * @return Emitted width in bits, or `0` when no width is known.
 */
static int rhs_actual_width(const IR_Expr *expr, const IR_Module *mod);

/**
 * @brief Emit one non-alias assignment inside a process.
 * @param cell_out Stream for helper cells and wires.
 * @param proc_out Stream for process-body statements.
 * @param mod Module that owns the assignment.
 * @param a Assignment to emit.
 * @param indent Indentation depth for process-body output.
 * @param pctx Process context for temporary assignment wires.
 */
static void emit_process_assignment(FILE *cell_out, FILE *proc_out,
                                    const IR_Module *mod,
                                    const IR_Assignment *a, int indent,
                                    const ProcessContext *pctx);

/**
 * @brief Emit a placeholder process comment for a memory write statement.
 * @param proc_out Destination process-body stream.
 * @param mod Module that owns the statement.
 * @param stmt Memory write statement being described.
 * @param indent Indentation depth for emitted output.
 */
static void emit_process_mem_write(FILE *proc_out, const IR_Module *mod,
                                   const IR_Stmt *stmt, int indent);

/**
 * @brief Emit an IF or ELIF subtree as RTLIL `switch` and `case` nodes.
 * @param cell_out Stream for helper cells and wires.
 * @param proc_out Stream for process-body statements.
 * @param mod Module that owns the control-flow subtree.
 * @param stmt IF statement subtree to emit.
 * @param indent Indentation depth for process-body output.
 * @param pctx Process context for temporary assignment wires.
 */
static void emit_process_if(FILE *cell_out, FILE *proc_out,
                            const IR_Module *mod, const IR_Stmt *stmt,
                            int indent, const ProcessContext *pctx);

/**
 * @brief Check whether a SELECT case body is empty.
 * @param sc Select-case descriptor to inspect.
 * @return Nonzero when the case body is absent or contains no statements.
 */
static int select_case_is_empty(const IR_SelectCase *sc);

/**
 * @brief Emit a SELECT subtree as RTLIL `switch` and `case` nodes.
 * @param cell_out Stream for helper cells and wires.
 * @param proc_out Stream for process-body statements.
 * @param mod Module that owns the control-flow subtree.
 * @param stmt SELECT statement subtree to emit.
 * @param indent Indentation depth for process-body output.
 * @param pctx Process context for temporary assignment wires.
 */
static void emit_process_select(FILE *cell_out, FILE *proc_out,
                                const IR_Module *mod, const IR_Stmt *stmt,
                                int indent, const ProcessContext *pctx);

/**
 * @brief Check whether an asynchronous block contains non-alias logic.
 * @param async_block Asynchronous statement subtree to inspect.
 * @return Nonzero when the block contains executable async logic.
 */
static int async_has_non_alias_logic(const IR_Stmt *async_block);

/**
 * @brief Emit `update` statements that copy temporaries back to signals.
 * @param out Destination stream.
 * @param mod Module that owns the updated signals.
 * @param assigned Set of assigned signal IDs.
 * @param pctx Process context that maps signals to temporaries.
 * @param indent Indentation depth for emitted statements.
 */
static void emit_updates(FILE *out, const IR_Module *mod,
                         const SignalIdList *assigned,
                         const ProcessContext *pctx, int indent);

/**
 * @brief Emit latch-retention defaults for asynchronous latch inference.
 * @param proc_out Destination process-body stream.
 * @param mod Module that owns the latch signals.
 * @param assigned Set of assigned signal IDs.
 * @param pctx Process context that maps signals to temporaries.
 * @param indent Indentation depth for emitted statements.
 */
static void emit_latch_defaults(FILE *proc_out, const IR_Module *mod,
                                const SignalIdList *assigned,
                                const ProcessContext *pctx, int indent);

/**
 * @brief Emit one clock-domain process.
 * @param out Destination RTLIL stream.
 * @param mod Module that owns the clock domain.
 * @param cd Clock domain to emit.
 */
static void emit_clock_domain_process(FILE *out, const IR_Module *mod,
                                      const IR_ClockDomain *cd);

static void siglist_init(SignalIdList *list) {
    list->ids = NULL;
    list->count = 0;
    list->capacity = 0;
}

static void siglist_free(SignalIdList *list) {
    if (list->ids) free(list->ids);
    list->ids = NULL;
    list->count = 0;
    list->capacity = 0;
}

static void siglist_add(SignalIdList *list, int id) {
    for (int i = 0; i < list->count; ++i) {
        if (list->ids[i] == id) return;
    }
    if (list->count >= list->capacity) {
        int new_cap = 0;
        size_t new_bytes = 0;
        int *new_ids = NULL;
        if (list->capacity == 0) {
            new_cap = 16;
        } else if (list->capacity > INT_MAX / 2) {
            return;
        } else {
            new_cap = list->capacity * 2;
        }
        if (new_cap <= list->capacity ||
            jz_size_mul_checked((size_t)new_cap, sizeof(int), &new_bytes) != 0) {
            return;
        }
        new_ids = realloc(list->ids, new_bytes);
        if (!new_ids) return;
        list->ids = new_ids;
        list->capacity = new_cap;
    }
    list->ids[list->count++] = id;
}

static void collect_assigned_signals(const IR_Stmt *stmt, SignalIdList *list)
{
    if (!stmt) return;
    switch (stmt->kind) {
    case STMT_ASSIGNMENT:
        if (!assignment_kind_is_alias(stmt->u.assign.kind)) {
            siglist_add(list, stmt->u.assign.lhs_signal_id);
        }
        break;
    case STMT_IF: {
        const IR_IfStmt *ifs = &stmt->u.if_stmt;
        collect_assigned_signals(ifs->then_block, list);
        if (ifs->elif_chain) collect_assigned_signals(ifs->elif_chain, list);
        if (ifs->else_block) collect_assigned_signals(ifs->else_block, list);
        break;
    }
    case STMT_SELECT: {
        const IR_SelectStmt *sel = &stmt->u.select_stmt;
        for (int i = 0; i < sel->num_cases; ++i) {
            collect_assigned_signals(sel->cases[i].body, list);
        }
        break;
    }
    case STMT_BLOCK: {
        const IR_BlockStmt *blk = &stmt->u.block;
        for (int i = 0; i < blk->count; ++i) {
            collect_assigned_signals(&blk->stmts[i], list);
        }
        break;
    }
    case STMT_MEM_WRITE:
        break;
    default:
        break;
    }
}

/* Emit default assignments at the start of a process body.
 * Each intermediate wire defaults to its real signal value so that
 * unassigned branches retain the current register/wire value.
 * For async (combinational) processes, only emit defaults for SIG_LATCH
 * signals (which need hold-value semantics for latch inference). */
static void emit_process_defaults(FILE *proc_out, const IR_Module *mod,
                                   const SignalIdList *assigned,
                                   const ProcessContext *pctx,
                                   int indent)
{
    for (int i = 0; i < assigned->count; ++i) {
        const IR_Signal *sig = rtlil_find_signal_by_id(mod, assigned->ids[i]);
        if (!sig || !sig->name) continue;
        const char *iname = pctx_find(pctx, assigned->ids[i]);
        if (!iname) continue;

        rtlil_indent(proc_out, indent);
        fprintf(proc_out, "assign %s \\%s\n", iname, sig->name);
    }
}

/* Build a ProcessContext from collected signal IDs, emitting intermediate
 * wire declarations at module level. */
static void build_process_context(FILE *out, const IR_Module *mod,
                                   const SignalIdList *assigned,
                                   ProcessContext *pctx)
{
    pctx_init(pctx);
    for (int i = 0; i < assigned->count; ++i) {
        const IR_Signal *sig = rtlil_find_signal_by_id(mod, assigned->ids[i]);
        if (!sig || !sig->name) continue;
        int w = sig->width > 0 ? sig->width : 1;

        /* Build intermediate wire name: $0\name[W-1:0] */
        char iname[128];
        snprintf(iname, sizeof(iname), "$0\\%s[%d:0]", sig->name, w - 1);

        /* Emit wire declaration at module level. */
        rtlil_indent(out, 1);
        if (w > 1) {
            fprintf(out, "wire width %d %s\n", w, iname);
        } else {
            fprintf(out, "wire %s\n", iname);
        }

        pctx_add(pctx, assigned->ids[i], iname);
    }
}

/* -------------------------------------------------------------------------
 * Helper: copy tmpfile contents to the real output
 * -------------------------------------------------------------------------
 */

static void flush_tmpfile_to(FILE *tmp, FILE *out)
{
    rewind(tmp);
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), tmp)) > 0) {
        fwrite(buf, 1, n, out);
    }
}

/* -------------------------------------------------------------------------
 * Assignment emission within a process
 *
 * In RTLIL processes, assignments are `assign \dest \src` (these become
 * defaults). When a ProcessContext is active, the LHS targets the
 * intermediate wire instead of the real signal.
 *
 * Cell/wire declarations from expression decomposition go to cell_out
 * (module level), while the assign statement itself goes to proc_out.
 * -------------------------------------------------------------------------
 */

static void emit_lhs_sigspec(FILE *proc_out, const IR_Module *mod,
                              const IR_Assignment *a, const IR_Signal *sig,
                              const ProcessContext *pctx)
{
    (void)mod;
    const char *iname = pctx ? pctx_find(pctx, a->lhs_signal_id) : NULL;

    if (iname) {
        /* Target intermediate wire. */
        if (a->is_sliced) {
            int width = a->lhs_msb - a->lhs_lsb + 1;
            if (width == 1) {
                fprintf(proc_out, "%s [%d]", iname, a->lhs_lsb);
            } else {
                fprintf(proc_out, "%s [%d:%d]", iname, a->lhs_msb, a->lhs_lsb);
            }
        } else {
            fprintf(proc_out, "%s", iname);
        }
    } else {
        /* Target real signal. */
        if (a->is_sliced) {
            int width = a->lhs_msb - a->lhs_lsb + 1;
            if (width == 1) {
                fprintf(proc_out, "\\%s [%d]", sig->name, a->lhs_lsb);
            } else {
                fprintf(proc_out, "\\%s [%d:%d]", sig->name, a->lhs_msb, a->lhs_lsb);
            }
        } else {
            fprintf(proc_out, "\\%s", sig->name);
        }
    }
}

static void emit_rhs_sigspec(FILE *cell_out, char *rhs_buf, int rhs_buf_size,
                              const IR_Module *mod,
                              const IR_Assignment *a, const IR_Signal *lhs_sig)
{
    if (!a->rhs) {
        /* NULL RHS: emit zero constant. */
        int width = lhs_sig->width > 0 ? lhs_sig->width : 1;
        if (a->is_sliced) {
            width = a->lhs_msb - a->lhs_lsb + 1;
        }
        int pos = snprintf(rhs_buf, rhs_buf_size, "%d'", width);
        for (int i = 0; i < width && pos < rhs_buf_size - 1; ++i) {
            rhs_buf[pos++] = '0';
        }
        rhs_buf[pos] = '\0';
        return;
    }

    /* Check for GND/VCC polymorphic constants. */
    if (a->rhs->kind == EXPR_LITERAL && a->rhs->const_name &&
        (strcmp(a->rhs->const_name, "GND") == 0 ||
         strcmp(a->rhs->const_name, "VCC") == 0)) {
        int width = lhs_sig->width;
        if (a->is_sliced) {
            width = a->lhs_msb - a->lhs_lsb + 1;
        }
        if (width <= 0) width = 1;
        uint64_t value = (strcmp(a->rhs->const_name, "VCC") == 0)
                       ? ((width < 64) ? ((1ULL << width) - 1) : ~0ULL)
                       : 0ULL;
        int pos = snprintf(rhs_buf, rhs_buf_size, "%d'", width);
        for (int i = width - 1; i >= 0 && pos < rhs_buf_size - 1; --i) {
            unsigned bit = (unsigned)((value >> i) & 1u);
            rhs_buf[pos++] = bit ? '1' : '0';
        }
        rhs_buf[pos] = '\0';
        return;
    }

    /* For plain literals, widen to match LHS width if needed.
     * RTLIL process assignments require matching widths; a width mismatch
     * (e.g. 8-bit literal assigned to 16-bit signal) can crash yosys. */
    if (a->rhs->kind == EXPR_LITERAL) {
        int lhs_width = lhs_sig->width > 0 ? lhs_sig->width : 1;
        if (a->is_sliced) {
            lhs_width = a->lhs_msb - a->lhs_lsb + 1;
        }
        int lit_width = a->rhs->u.literal.literal.width;
        if (lit_width > 0 && lit_width < lhs_width) {
            /* Emit literal zero-extended to the LHS width. */
            int pos = snprintf(rhs_buf, rhs_buf_size, "%d'", lhs_width);
            for (int i = lhs_width - 1; i >= 0 && pos < rhs_buf_size - 1; --i) {
                int wi = i / 64;
                int bi = i % 64;
                unsigned bit = (wi < IR_LIT_WORDS) ? (unsigned)((a->rhs->u.literal.literal.words[wi] >> bi) & 1u) : 0;
                rhs_buf[pos++] = bit ? '1' : '0';
            }
            rhs_buf[pos] = '\0';
            return;
        }
    }

    /* For complex expressions, decompose into cells (written to cell_out)
     * and return the result sigspec in rhs_buf. */
    rtlil_emit_expr(cell_out, mod, a->rhs, rhs_buf, rhs_buf_size);
}

/* Determine the actual width of an RHS expression as it will appear in
 * the RTLIL sigspec.  For signal refs, use the real signal width (which
 * may differ from the expression's width after template specialization).
 * This is used to detect width mismatches between the RHS sigspec and the
 * LHS of a process assignment. */
static int rhs_actual_width(const IR_Expr *expr, const IR_Module *mod)
{
    if (!expr) return 0;
    if (expr->kind == EXPR_SIGNAL_REF) {
        const IR_Signal *sig = rtlil_find_signal_by_id(mod, expr->u.signal_ref.signal_id);
        if (sig && sig->width > 0) return sig->width;
    }
    return expr->width > 0 ? expr->width : 0;
}

static void emit_process_assignment(FILE *cell_out, FILE *proc_out,
                                     const IR_Module *mod,
                                     const IR_Assignment *a, int indent,
                                     const ProcessContext *pctx)
{
    if (!a) return;

    /* Skip alias assignments — they're emitted as top-level connects. */
    if (assignment_kind_is_alias(a->kind)) return;

    const IR_Signal *lhs_sig = rtlil_find_signal_by_id(mod, a->lhs_signal_id);
    if (!lhs_sig || !lhs_sig->name) return;

    /* Emit RHS expression cells to cell_out, get result sigspec. */
    char rhs_ss[RTLIL_SIGSPEC_MAX];
    emit_rhs_sigspec(cell_out, rhs_ss, sizeof(rhs_ss), mod, a, lhs_sig);

    /* Determine LHS width. */
    int lhs_width = lhs_sig->width > 0 ? lhs_sig->width : 1;
    if (a->is_sliced) {
        lhs_width = a->lhs_msb - a->lhs_lsb + 1;
    }

    /* If the RHS expression is narrower than the LHS, emit a zero-extend
     * cell so that RTLIL process assign widths match.  Width mismatches
     * crash yosys proc_prune.
     * Skip for literals — emit_rhs_sigspec already widens those. */
    int rhs_w = rhs_actual_width(a->rhs, mod);
    int is_literal = (a->rhs && a->rhs->kind == EXPR_LITERAL);
    if (rhs_w > 0 && rhs_w < lhs_width && !is_literal) {
        /* Emit: wire width <lhs_width> $auto$N
         *       cell $pos $auto$M
         *         parameter \A_SIGNED 0
         *         parameter \A_WIDTH <rhs_w>
         *         parameter \Y_WIDTH <lhs_width>
         *         connect \A <rhs_ss>
         *         connect \Y $auto$N
         *       end */
        char ext_wire[128];
        int ext_wire_id = rtlil_next_id();
        snprintf(ext_wire, sizeof(ext_wire), "$auto$%d", ext_wire_id);
        rtlil_indent(cell_out, 1);
        fprintf(cell_out, "wire width %d %s\n", lhs_width, ext_wire);

        int ext_cell_id = rtlil_next_id();
        rtlil_indent(cell_out, 1);
        fprintf(cell_out, "cell $pos $auto$%d\n", ext_cell_id);
        rtlil_indent(cell_out, 2);
        fprintf(cell_out, "parameter \\A_SIGNED 0\n");
        rtlil_indent(cell_out, 2);
        fprintf(cell_out, "parameter \\A_WIDTH %d\n", rhs_w);
        rtlil_indent(cell_out, 2);
        fprintf(cell_out, "parameter \\Y_WIDTH %d\n", lhs_width);
        rtlil_indent(cell_out, 2);
        fprintf(cell_out, "connect \\A %s\n", rhs_ss);
        rtlil_indent(cell_out, 2);
        fprintf(cell_out, "connect \\Y %s\n", ext_wire);
        rtlil_indent(cell_out, 1);
        fprintf(cell_out, "end\n");

        snprintf(rhs_ss, sizeof(rhs_ss), "%s", ext_wire);
    }

    /* Emit assign statement to proc_out. */
    rtlil_indent(proc_out, indent);
    fputs("assign ", proc_out);
    emit_lhs_sigspec(proc_out, mod, a, lhs_sig, pctx);
    fprintf(proc_out, " %s\n", rhs_ss);
}

/* -------------------------------------------------------------------------
 * Memory write emission within a process
 * -------------------------------------------------------------------------
 */

static void emit_process_mem_write(FILE *proc_out, const IR_Module *mod,
                                    const IR_Stmt *stmt, int indent)
{
    (void)mod;
    rtlil_indent(proc_out, indent);
    fprintf(proc_out, "# memory write: %s (handled by $memwr_v2 cell)\n",
            stmt->u.mem_write.memory_name ? stmt->u.mem_write.memory_name : "unknown");
}

/* -------------------------------------------------------------------------
 * IF/ELIF/ELSE → switch/case emission
 *
 * RTLIL IF is represented as:
 *   switch <condition_sigspec>
 *     case 1'1
 *       <then body>
 *     case
 *       <else body>
 *   end
 *
 * Condition expression cells go to cell_out (module level).
 * switch/case structure goes to proc_out (process body).
 * -------------------------------------------------------------------------
 */

static void emit_process_if(FILE *cell_out, FILE *proc_out,
                              const IR_Module *mod,
                              const IR_Stmt *stmt, int indent,
                              const ProcessContext *pctx)
{
    if (!stmt || stmt->kind != STMT_IF) return;
    const IR_IfStmt *ifs = &stmt->u.if_stmt;

    /* Emit the condition as cells (to cell_out), get the sigspec. */
    char cond_ss[RTLIL_SIGSPEC_MAX];
    rtlil_emit_expr(cell_out, mod, ifs->condition, cond_ss, sizeof(cond_ss));

    /* Reduce to 1-bit if wider. */
    int cond_width = ifs->condition ? ifs->condition->width : 1;
    if (cond_width > 1) {
        int id = rtlil_next_id();
        char wire_name[128];
        snprintf(wire_name, sizeof(wire_name), "$auto$%d", id);
        rtlil_indent(cell_out, 1);
        fprintf(cell_out, "wire width 1 %s\n", wire_name);

        id = rtlil_next_id();
        rtlil_indent(cell_out, 1);
        fprintf(cell_out, "cell $reduce_or $auto$%d\n", id);
        rtlil_indent(cell_out, 2);
        fprintf(cell_out, "parameter \\A_SIGNED 0\n");
        rtlil_indent(cell_out, 2);
        fprintf(cell_out, "parameter \\A_WIDTH %d\n", cond_width);
        rtlil_indent(cell_out, 2);
        fprintf(cell_out, "parameter \\Y_WIDTH 1\n");
        rtlil_indent(cell_out, 2);
        fprintf(cell_out, "connect \\A %s\n", cond_ss);
        rtlil_indent(cell_out, 2);
        fprintf(cell_out, "connect \\Y %s\n", wire_name);
        rtlil_indent(cell_out, 1);
        fprintf(cell_out, "end\n");
        snprintf(cond_ss, sizeof(cond_ss), "%s", wire_name);
    }

    rtlil_indent(proc_out, indent);
    fprintf(proc_out, "switch %s\n", cond_ss);

    /* Then block (case 1'1). */
    rtlil_indent(proc_out, indent + 1);
    fprintf(proc_out, "case 1'1\n");
    if (ifs->then_block) {
        emit_process_stmt(cell_out, proc_out, mod, ifs->then_block, indent + 2, pctx);
    }

    /* Handle elif chain: each elif becomes a nested switch in the else.
     * The IR stores else_block on the ROOT if node, not the last elif,
     * so we must emit it here after the elif recursion completes. */
    if (ifs->elif_chain && ifs->elif_chain->kind == STMT_IF) {
        rtlil_indent(proc_out, indent + 1);
        fprintf(proc_out, "case\n");
        /* The elif itself may have a trailing else. Walk the elif chain
         * to find the terminal node and attach our else_block to it so
         * the recursive call emits it at the right nesting level. */
        if (ifs->else_block) {
            /* Find the last elif in the chain. */
            IR_Stmt *last = ifs->elif_chain;
            while (last->kind == STMT_IF &&
                   last->u.if_stmt.elif_chain &&
                   last->u.if_stmt.elif_chain->kind == STMT_IF) {
                last = last->u.if_stmt.elif_chain;
            }
            /* Attach the root's else_block if the terminal elif
             * doesn't already have one. */
            if (!last->u.if_stmt.else_block) {
                last->u.if_stmt.else_block = ifs->else_block;
            }
        }
        emit_process_if(cell_out, proc_out, mod, ifs->elif_chain, indent + 2, pctx);
    } else if (ifs->else_block) {
        /* Simple else. */
        rtlil_indent(proc_out, indent + 1);
        fprintf(proc_out, "case\n");
        emit_process_stmt(cell_out, proc_out, mod, ifs->else_block, indent + 2, pctx);
    } else {
        /* No else - empty default case. */
        rtlil_indent(proc_out, indent + 1);
        fprintf(proc_out, "case\n");
    }

    rtlil_indent(proc_out, indent);
    fprintf(proc_out, "end\n");
}

/* -------------------------------------------------------------------------
 * SELECT → switch/case emission
 * -------------------------------------------------------------------------
 */

/* Check if a SELECT case body is empty (NULL or empty block). */
static int select_case_is_empty(const IR_SelectCase *sc)
{
    if (!sc->body) return 1;
    if (sc->body->kind == STMT_BLOCK && sc->body->u.block.count == 0) return 1;
    return 0;
}

static void emit_process_select(FILE *cell_out, FILE *proc_out,
                                  const IR_Module *mod,
                                  const IR_Stmt *stmt, int indent,
                                  const ProcessContext *pctx)
{
    if (!stmt || stmt->kind != STMT_SELECT) return;
    const IR_SelectStmt *sel = &stmt->u.select_stmt;

    /* Selector expression cells go to cell_out. */
    char sel_ss[RTLIL_SIGSPEC_MAX];
    rtlil_emit_expr(cell_out, mod, sel->selector, sel_ss, sizeof(sel_ss));

    rtlil_indent(proc_out, indent);
    fprintf(proc_out, "switch %s\n", sel_ss);

    for (int i = 0; i < sel->num_cases; ++i) {
        const IR_SelectCase *sc = &sel->cases[i];

        /* Detect fall-through: if this case has a value but no body,
         * merge it with subsequent cases into a multi-value case line.
         * RTLIL case labels do NOT fall through, so we must use the
         * comma-separated multi-value syntax: case V1, V2, V3 */
        if (sc->case_value && select_case_is_empty(sc)) {
            /* Collect all consecutive fall-through values. */
            rtlil_indent(proc_out, indent + 1);
            fputs("case ", proc_out);

            char val_ss[RTLIL_SIGSPEC_MAX];
            rtlil_emit_expr(cell_out, mod, sc->case_value,
                            val_ss, sizeof(val_ss));
            fputs(val_ss, proc_out);

            /* Merge subsequent fall-through cases. */
            while (i + 1 < sel->num_cases) {
                const IR_SelectCase *next = &sel->cases[i + 1];
                if (!next->case_value) break; /* DEFAULT */

                char next_ss[RTLIL_SIGSPEC_MAX];
                rtlil_emit_expr(cell_out, mod, next->case_value,
                                next_ss, sizeof(next_ss));
                fprintf(proc_out, ", %s", next_ss);
                ++i;

                if (!select_case_is_empty(next)) {
                    /* This case has a body — emit it. */
                    fputc('\n', proc_out);
                    emit_process_stmt(cell_out, proc_out, mod,
                                      next->body, indent + 2, pctx);
                    goto next_case;
                }
            }
            fputc('\n', proc_out);
            next_case:;
            continue;
        }

        rtlil_indent(proc_out, indent + 1);
        if (!sc->case_value) {
            /* Default case. */
            fprintf(proc_out, "case\n");
        } else {
            /* Emit case value — cells to cell_out, sigspec for the case label. */
            char val_ss[RTLIL_SIGSPEC_MAX];
            rtlil_emit_expr(cell_out, mod, sc->case_value, val_ss, sizeof(val_ss));
            fprintf(proc_out, "case %s\n", val_ss);
        }

        if (sc->body) {
            emit_process_stmt(cell_out, proc_out, mod, sc->body, indent + 2, pctx);
        }
    }

    rtlil_indent(proc_out, indent);
    fprintf(proc_out, "end\n");
}

/* -------------------------------------------------------------------------
 * Generic statement dispatch
 * -------------------------------------------------------------------------
 */

static void emit_process_block(FILE *cell_out, FILE *proc_out,
                                const IR_Module *mod,
                                const IR_Stmt *block, int indent,
                                const ProcessContext *pctx)
{
    if (!block || block->kind != STMT_BLOCK) return;
    const IR_BlockStmt *blk = &block->u.block;
    for (int i = 0; i < blk->count; ++i) {
        emit_process_stmt(cell_out, proc_out, mod, &blk->stmts[i], indent, pctx);
    }
}

static void emit_process_stmt(FILE *cell_out, FILE *proc_out,
                               const IR_Module *mod,
                               const IR_Stmt *stmt, int indent,
                               const ProcessContext *pctx)
{
    if (!stmt) return;

    switch (stmt->kind) {
    case STMT_ASSIGNMENT:
        emit_process_assignment(cell_out, proc_out, mod, &stmt->u.assign, indent, pctx);
        break;
    case STMT_IF:
        emit_process_if(cell_out, proc_out, mod, stmt, indent, pctx);
        break;
    case STMT_SELECT:
        emit_process_select(cell_out, proc_out, mod, stmt, indent, pctx);
        break;
    case STMT_BLOCK:
        emit_process_block(cell_out, proc_out, mod, stmt, indent, pctx);
        break;
    case STMT_MEM_WRITE:
        emit_process_mem_write(proc_out, mod, stmt, indent);
        break;
    default:
        rtlil_indent(proc_out, indent);
        fprintf(proc_out, "# unsupported stmt kind %d\n", (int)stmt->kind);
        break;
    }
}

/* -------------------------------------------------------------------------
 * Helper: check if async block has non-alias logic
 * -------------------------------------------------------------------------
 */

static int async_has_non_alias_logic(const IR_Stmt *async_block)
{
    if (!async_block || async_block->kind != STMT_BLOCK) return 0;
    const IR_BlockStmt *blk = &async_block->u.block;
    for (int i = 0; i < blk->count; ++i) {
        const IR_Stmt *child = &blk->stmts[i];
        if (child->kind != STMT_ASSIGNMENT) return 1;
        if (!assignment_kind_is_alias(child->u.assign.kind)) return 1;
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * Emit update statements for all signals in the process context
 * -------------------------------------------------------------------------
 */

static void emit_updates(FILE *out, const IR_Module *mod,
                          const SignalIdList *assigned,
                          const ProcessContext *pctx, int indent)
{
    for (int i = 0; i < assigned->count; ++i) {
        const IR_Signal *sig = rtlil_find_signal_by_id(mod, assigned->ids[i]);
        if (!sig || !sig->name) continue;
        const char *iname = pctx_find(pctx, assigned->ids[i]);
        if (!iname) continue;
        rtlil_indent(out, indent);
        fprintf(out, "update \\%s %s\n", sig->name, iname);
    }
}

/* -------------------------------------------------------------------------
 * Async block emission
 *
 * The async block becomes:
 *   wire $0\sig[W-1:0]          (intermediate wires, at module level)
 *   <cells from expression decomposition, at module level>
 *   process $proc$async$N
 *     assign $0\sig[W-1:0] \src (body targets intermediates)
 *     sync always
 *       update \sig $0\sig[W-1:0]
 *   end
 * -------------------------------------------------------------------------
 */

/* Emit default assignments only for SIG_LATCH signals in the async block.
 * Latch signals need `assign $0\sig \sig` to express hold-value semantics,
 * which yosys proc_dlatch recognizes as a D-latch. Non-latch signals must
 * NOT get defaults (that would create unintended latches). */
static void emit_latch_defaults(FILE *proc_out, const IR_Module *mod,
                                 const SignalIdList *assigned,
                                 const ProcessContext *pctx,
                                 int indent)
{
    for (int i = 0; i < assigned->count; ++i) {
        /* Use raw lookup to check the original signal kind (before aliasing). */
        const IR_Signal *raw = rtlil_find_signal_by_id_raw(mod, assigned->ids[i]);
        if (!raw || raw->kind != SIG_LATCH) continue;

        const IR_Signal *sig = rtlil_find_signal_by_id(mod, assigned->ids[i]);
        if (!sig || !sig->name) continue;
        const char *iname = pctx_find(pctx, assigned->ids[i]);
        if (!iname) continue;

        rtlil_indent(proc_out, indent);
        fprintf(proc_out, "assign %s \\%s\n", iname, sig->name);
    }
}

void rtlil_emit_async_block(FILE *out, const IR_Module *mod)
{
    if (!out || !mod || !mod->async_block) return;
    if (!async_has_non_alias_logic(mod->async_block)) return;

    /* Collect all signals assigned in the async block. */
    SignalIdList assigned;
    siglist_init(&assigned);
    collect_assigned_signals(mod->async_block, &assigned);

    /* Build process context with intermediate wires (emitted to out). */
    ProcessContext pctx;
    build_process_context(out, mod, &assigned, &pctx);

    /* Buffer the process body separately. Expression cells/wires go to
     * 'out' (module level), while process body goes to the temp buffer. */
    FILE *proc_out = tmpfile();
    if (!proc_out) {
        /* Fallback: emit everything to out (cells inside process — not ideal
         * but better than crashing). */
        proc_out = out;
    }

    /* Emit the body — cells to 'out', process body to 'proc_out'. */
    emit_process_stmt(out, proc_out, mod, mod->async_block, 2, &pctx);

    /* Now emit the process with the buffered body. */
    int proc_id = rtlil_next_id();
    rtlil_indent(out, 1);
    fprintf(out, "process $proc$async$%d\n", proc_id);

    /* Emit defaults for SIG_LATCH signals only. This creates hold-value
     * semantics that yosys proc_dlatch will recognize as D-latches.
     * Non-latch signals intentionally get no defaults (pure combinational). */
    emit_latch_defaults(out, mod, &assigned, &pctx, 2);

    /* Flush buffered process body. */
    if (proc_out != out) {
        flush_tmpfile_to(proc_out, out);
        fclose(proc_out);
    }

    /* Sync always block with update statements. */
    rtlil_indent(out, 2);
    fprintf(out, "sync always\n");
    emit_updates(out, mod, &assigned, &pctx, 3);

    rtlil_indent(out, 1);
    fprintf(out, "end\n");

    pctx_free(&pctx);
    siglist_free(&assigned);
}

/* -------------------------------------------------------------------------
 * Clock domain emission
 *
 * Each clock domain becomes:
 *   wire $0\reg[W-1:0]          (intermediate wires)
 *   <cells from expression decomposition>
 *   process $proc$clkN$M
 *     assign $0\reg[W-1:0] \src (body targets intermediates)
 *     sync posedge|negedge \clk
 *       update \reg $0\reg[W-1:0]
 *     sync high|low \rst        (optional async reset)
 *       update \reg <reset_val>
 *     sync init
 *       update \reg <init_val>
 *   end
 * -------------------------------------------------------------------------
 */

static void emit_clock_domain_process(FILE *out, const IR_Module *mod,
                                       const IR_ClockDomain *cd)
{
    if (!out || !mod || !cd) return;

    /* Collect all signals assigned in this clock domain. */
    SignalIdList assigned;
    siglist_init(&assigned);
    if (cd->statements) {
        collect_assigned_signals(cd->statements, &assigned);
    }

    /* Build process context with intermediate wires (emitted to out). */
    ProcessContext pctx;
    build_process_context(out, mod, &assigned, &pctx);

    /* Buffer the process body. Cells go to 'out', body to tmpfile. */
    FILE *proc_out = tmpfile();
    if (!proc_out) {
        proc_out = out;
    }

    /* Emit the body, targeting intermediates. */
    if (cd->statements) {
        emit_process_stmt(out, proc_out, mod, cd->statements, 2, &pctx);
    }

    /* Emit the process with buffered body. */
    int proc_id = rtlil_next_id();
    rtlil_indent(out, 1);
    fprintf(out, "process $proc$clk%d$%d\n", cd->id, proc_id);

    /* Default assignments: intermediate = current register value.
     * This ensures unmatched conditional branches retain the old value. */
    emit_process_defaults(out, mod, &assigned, &pctx, 2);

    /* Flush buffered process body. */
    if (proc_out != out) {
        flush_tmpfile_to(proc_out, out);
        fclose(proc_out);
    }

    /* Sync block for clock edge. */
    if (cd->num_sensitivity > 0 && cd->sensitivity_list) {
        /* Primary clock sync. */
        const IR_SensitivityEntry *clk_entry = &cd->sensitivity_list[0];
        const IR_Signal *clk_sig = rtlil_find_signal_by_id(mod, clk_entry->signal_id);
        const char *edge = (clk_entry->edge == EDGE_FALLING) ? "negedge" : "posedge";

        rtlil_indent(out, 2);
        fprintf(out, "sync %s \\%s\n", edge,
                clk_sig && clk_sig->name ? clk_sig->name : "clk");

        /* Update from intermediates. */
        emit_updates(out, mod, &assigned, &pctx, 3);

        /* Emit sync high/low for true async resets (no synchronizer).
         * When a synchronizer is used (reset_sync_signal_id >= 0), the body
         * switch references the synchronized signal, not the raw reset, so
         * skip sync high/low to avoid yosys proc_arst mismatches. */
        if (cd->reset_signal_id >= 0 && cd->reset_type == RESET_IMMEDIATE &&
            cd->reset_sync_signal_id < 0) {
            const IR_Signal *rst_sig = NULL;
            for (int s = 1; s < cd->num_sensitivity; ++s) {
                if (cd->sensitivity_list[s].signal_id == cd->reset_signal_id) {
                    rst_sig = rtlil_find_signal_by_id(mod, cd->sensitivity_list[s].signal_id);
                    break;
                }
            }
            if (rst_sig && rst_sig->name) {
                const char *rst_level = (cd->reset_active == RESET_ACTIVE_LOW)
                                      ? "low" : "high";
                rtlil_indent(out, 2);
                fprintf(out, "sync %s \\%s\n", rst_level, rst_sig->name);

                for (int r = 0; r < cd->num_registers; ++r) {
                    const IR_Signal *reg = rtlil_find_signal_by_id(mod, cd->register_ids[r]);
                    if (!reg || !reg->name) continue;
                    if (reg->kind != SIG_REGISTER) continue;

                    rtlil_indent(out, 3);
                    fprintf(out, "update \\%s ", reg->name);

                    if (reg->u.reg.reset_value_gnd_vcc) {
                        int width = reg->width > 0 ? reg->width : 1;
                        uint64_t val = 0;
                        if (strcmp(reg->u.reg.reset_value_gnd_vcc, "VCC") == 0) {
                            val = (width < 64) ? ((1ULL << width) - 1) : ~0ULL;
                        }
                        rtlil_emit_const_val(out, width, val);
                    } else {
                        rtlil_emit_const(out, &reg->u.reg.reset_value);
                    }
                    fputc('\n', out);
                }
            }
        }
    }

    rtlil_indent(out, 1);
    fprintf(out, "end\n");

    pctx_free(&pctx);
    siglist_free(&assigned);
}

void rtlil_emit_clock_domains(FILE *out, const IR_Module *mod)
{
    if (!out || !mod || mod->num_clock_domains <= 0) return;

    for (int i = 0; i < mod->num_clock_domains; ++i) {
        emit_clock_domain_process(out, mod, &mod->clock_domains[i]);
    }
}
