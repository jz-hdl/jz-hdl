/*
 * emit_decl.c - Declaration emission for the Verilog-2005 backend.
 *
 * This file handles emitting module headers, port declarations, signal
 * declarations, and memory declarations.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "verilog_internal.h"
#include "ir.h"
#include "util.h"

/* -------------------------------------------------------------------------
 * Helper: determine if a statement assigns to a given signal
 * -------------------------------------------------------------------------
 */

static int stmt_assigns_to_signal(const IR_Stmt *stmt, int signal_id)
{
    if (!stmt) return 0;

    switch (stmt->kind) {
    case STMT_ASSIGNMENT:
        if (assignment_kind_is_alias(stmt->u.assign.kind)) return 0;
        return (stmt->u.assign.lhs_signal_id == signal_id);

    case STMT_IF: {
        const IR_IfStmt *ifs = &stmt->u.if_stmt;
        if (stmt_assigns_to_signal(ifs->then_block, signal_id)) return 1;
        const IR_Stmt *elif = ifs->elif_chain;
        while (elif && elif->kind == STMT_IF) {
            const IR_IfStmt *eifs = &elif->u.if_stmt;
            if (stmt_assigns_to_signal(eifs->then_block, signal_id)) return 1;
            elif = eifs->elif_chain;
        }
        if (stmt_assigns_to_signal(ifs->else_block, signal_id)) return 1;
        return 0;
    }

    case STMT_SELECT: {
        const IR_SelectStmt *sel = &stmt->u.select_stmt;
        for (int i = 0; i < sel->num_cases; ++i) {
            if (stmt_assigns_to_signal(sel->cases[i].body, signal_id)) return 1;
        }
        return 0;
    }

    case STMT_BLOCK: {
        const IR_BlockStmt *blk = &stmt->u.block;
        for (int i = 0; i < blk->count; ++i) {
            if (stmt_assigns_to_signal(&blk->stmts[i], signal_id)) return 1;
        }
        return 0;
    }

    default:
        return 0;
    }
}

static size_t stmt_traversal_node_count(const IR_Stmt *stmt)
{
    size_t count = 0;
    size_t added = 0;

    if (!stmt) {
        return 0;
    }

    count = 1;
    switch (stmt->kind) {
    case STMT_IF: {
        const IR_IfStmt *ifs = &stmt->u.if_stmt;
        added = stmt_traversal_node_count(ifs->then_block);
        if (jz_size_add_checked(count, added, &count) != 0) return SIZE_MAX;
        added = stmt_traversal_node_count(ifs->else_block);
        if (jz_size_add_checked(count, added, &count) != 0) return SIZE_MAX;
        for (const IR_Stmt *elif = ifs->elif_chain;
             elif && elif->kind == STMT_IF;
             elif = elif->u.if_stmt.elif_chain) {
            added = stmt_traversal_node_count(elif->u.if_stmt.then_block);
            if (jz_size_add_checked(count, added, &count) != 0) return SIZE_MAX;
        }
        break;
    }
    case STMT_SELECT: {
        const IR_SelectStmt *sel = &stmt->u.select_stmt;
        for (int i = 0; i < sel->num_cases; ++i) {
            added = stmt_traversal_node_count(sel->cases[i].body);
            if (jz_size_add_checked(count, added, &count) != 0) return SIZE_MAX;
        }
        break;
    }
    case STMT_BLOCK: {
        const IR_BlockStmt *blk = &stmt->u.block;
        for (int i = 0; i < blk->count; ++i) {
            added = stmt_traversal_node_count(&blk->stmts[i]);
            if (jz_size_add_checked(count, added, &count) != 0) return SIZE_MAX;
        }
        break;
    }
    default:
        break;
    }

    return count;
}

/* -------------------------------------------------------------------------
 * Helper: determine if a port needs to be declared as reg
 * -------------------------------------------------------------------------
 */

int module_port_needs_reg(const IR_Module *mod, int signal_id)
{
    size_t stack_cap = 0;
    size_t stack_bytes = 0;
    IR_Stmt **stack = NULL;
    size_t top = 0;

    if (!mod) return 0;

    stack_cap = mod->async_block ? stmt_traversal_node_count(mod->async_block) : 0u;
    for (int i = 0; i < mod->num_clock_domains; ++i) {
        const IR_ClockDomain *cd = &mod->clock_domains[i];
        size_t added = stmt_traversal_node_count(cd->statements);
        if (jz_size_add_checked(stack_cap, added, &stack_cap) != 0) {
            return 0;
        }
    }
    if (stack_cap == 0) {
        return 0;
    }
    if (jz_size_mul_checked(stack_cap, sizeof(IR_Stmt *), &stack_bytes) != 0) {
        return 0;
    }
    stack = (IR_Stmt **)malloc(stack_bytes);
    if (!stack) {
        return 0;
    }

    if (mod->async_block) {
        stack[top++] = mod->async_block;
    }
    for (int i = 0; i < mod->num_clock_domains; ++i) {
        const IR_ClockDomain *cd = &mod->clock_domains[i];
        if (cd->statements) {
            stack[top++] = cd->statements;
        }
    }

    /* Pre-resolve the target signal through aliasing so we can match
     * assignments that target an aliased signal (e.g., bin_dest -> data_out).
     */
    const IR_Signal *target_sig = find_signal_by_id(mod, signal_id);

    while (top > 0) {
        IR_Stmt *stmt = stack[--top];
        if (!stmt) continue;

        switch (stmt->kind) {
        case STMT_ASSIGNMENT: {
            const IR_Assignment *a = &stmt->u.assign;
            if (!assignment_kind_is_alias(a->kind)) {
                /* Check direct match or alias-resolved match */
                if (a->lhs_signal_id == signal_id) {
                    return 1;
                }
                if (target_sig) {
                    const IR_Signal *lhs_sig = find_signal_by_id(mod, a->lhs_signal_id);
                    if (lhs_sig && lhs_sig == target_sig) {
                        return 1;
                    }
                }
            }
            break;
        }
        case STMT_IF: {
            const IR_IfStmt *ifs = &stmt->u.if_stmt;
            if (ifs->then_block) stack[top++] = ifs->then_block;
            IR_Stmt *elif = ifs->elif_chain;
            while (elif && elif->kind == STMT_IF) {
                const IR_IfStmt *eifs = &elif->u.if_stmt;
                if (eifs->then_block) stack[top++] = eifs->then_block;
                elif = eifs->elif_chain;
            }
            if (ifs->else_block) stack[top++] = ifs->else_block;
            break;
        }
        case STMT_SELECT: {
            const IR_SelectStmt *sel = &stmt->u.select_stmt;
            for (int i = 0; i < sel->num_cases; ++i) {
                if (sel->cases[i].body) {
                    stack[top++] = sel->cases[i].body;
                }
            }
            break;
        }
        case STMT_BLOCK: {
            const IR_BlockStmt *blk = &stmt->u.block;
            for (int i = 0; i < blk->count; ++i) {
                stack[top++] = &blk->stmts[i];
            }
            break;
        }
        default:
            break;
        }
    }

    free(stack);
    return 0;
}

/* -------------------------------------------------------------------------
 * Helper: determine if a signal is written in any block
 * -------------------------------------------------------------------------
 */

int module_signal_is_written(const IR_Module *mod, int signal_id)
{
    if (!mod) return 0;

    if (mod->async_block && stmt_assigns_to_signal(mod->async_block, signal_id)) {
        return 1;
    }
    for (int i = 0; i < mod->num_clock_domains; ++i) {
        const IR_ClockDomain *cd = &mod->clock_domains[i];
        if (cd->statements && stmt_assigns_to_signal(cd->statements, signal_id)) {
            return 1;
        }
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * Module header emission
 * -------------------------------------------------------------------------
 */

void emit_module_header(FILE *out, const IR_Module *mod)
{
    const char *name = (mod->name && mod->name[0] != '\0') ? mod->name : "jz_unnamed_module";

    int num_ports = 0;
    for (int i = 0; i < mod->num_signals; ++i) {
        if (mod->signals[i].kind == SIG_PORT) {
            ++num_ports;
        }
    }

    if (num_ports == 0) {
        fprintf(out, "module %s;\n", name);
        return;
    }

    fprintf(out, "module %s (\n", name);

    int seen = 0;
    for (int i = 0; i < mod->num_signals; ++i) {
        IR_Signal *sig = &mod->signals[i];
        if (sig->kind != SIG_PORT) {
            continue;
        }
        char esc[256];
        const char *sn = verilog_safe_name(sig->name ? sig->name : "jz_unnamed_port", esc, (int)sizeof(esc));
        fprintf(out, "    %s%s\n",
                sn,
                (seen + 1 < num_ports) ? "," : "");
        ++seen;
    }

    fprintf(out, ");\n");
}

/* -------------------------------------------------------------------------
 * Port declaration emission
 * -------------------------------------------------------------------------
 */

void emit_port_declarations(FILE *out, const IR_Module *mod)
{
    fprintf(out, "    // Ports\n");

    for (int i = 0; i < mod->num_signals; ++i) {
        const IR_Signal *sig = &mod->signals[i];
        if (sig->kind != SIG_PORT) {
            continue;
        }

        const char *dir = "input";
        const char *extra = "";
        switch (sig->u.port.direction) {
            case PORT_IN:
                dir = "input";
                break;
            case PORT_OUT: {
                dir = "output";
                if (module_port_needs_reg(mod, sig->id)) {
                    extra = " reg";
                }
                break;
            }
            case PORT_INOUT:
                dir = "inout";
                break;
            default:
                dir = "input";
                break;
        }

        fprintf(out, "    %s%s", dir, extra);
        emit_width_range(out, sig->width);
        char esc2[256];
        const char *sn2 = verilog_safe_name(sig->name ? sig->name : "jz_unnamed_port", esc2, (int)sizeof(esc2));
        fprintf(out, " %s;\n", sn2);
    }

    fputc('\n', out);
}

/* -------------------------------------------------------------------------
 * Helper: collect signal IDs used as SELECT selectors in sync blocks
 * -------------------------------------------------------------------------
 */

static void collect_select_signals_from_stmt(const IR_Stmt *stmt, int *ids, int *count, int cap)
{
    if (!stmt) return;

    switch (stmt->kind) {
    case STMT_SELECT: {
        const IR_SelectStmt *sel = &stmt->u.select_stmt;
        if (sel->selector && sel->selector->kind == EXPR_SIGNAL_REF && *count < cap) {
            int sid = sel->selector->u.signal_ref.signal_id;
            /* Avoid duplicates */
            bool found = false;
            for (int i = 0; i < *count; i++) {
                if (ids[i] == sid) { found = true; break; }
            }
            if (!found) ids[(*count)++] = sid;
        }
        /* Recurse into case bodies */
        for (int i = 0; i < sel->num_cases; i++) {
            collect_select_signals_from_stmt(sel->cases[i].body, ids, count, cap);
        }
        break;
    }
    case STMT_IF: {
        const IR_IfStmt *ifs = &stmt->u.if_stmt;
        collect_select_signals_from_stmt(ifs->then_block, ids, count, cap);
        collect_select_signals_from_stmt(ifs->else_block, ids, count, cap);
        break;
    }
    case STMT_BLOCK: {
        const IR_BlockStmt *blk = &stmt->u.block;
        for (int i = 0; i < blk->count; i++) {
            collect_select_signals_from_stmt(&blk->stmts[i], ids, count, cap);
        }
        break;
    }
    default:
        break;
    }
}

static bool signal_is_select_selector(const IR_Module *mod, int signal_id)
{
    int ids[64];
    int count = 0;

    for (int d = 0; d < mod->num_clock_domains; d++) {
        const IR_ClockDomain *cd = &mod->clock_domains[d];
        if (cd->statements) {
            collect_select_signals_from_stmt(cd->statements, ids, &count, 64);
        }
    }

    for (int i = 0; i < count; i++) {
        if (ids[i] == signal_id) return true;
    }
    return false;
}

/* -------------------------------------------------------------------------
 * Internal signal declaration emission
 * -------------------------------------------------------------------------
 */

void emit_internal_signal_declarations(FILE *out, const IR_Module *mod)
{
    fprintf(out, "    // Signals\n");

    for (int i = 0; i < mod->num_signals; ++i) {
        const IR_Signal *sig = &mod->signals[i];
        if (sig->kind == SIG_PORT) {
            continue;
        }

        if (sig->kind == SIG_NET && !alias_ctx_is_representative(mod, i)) {
            continue;
        }

        const char *kw = NULL;
        switch (sig->kind) {
            case SIG_NET:
                kw = module_signal_is_written(mod, sig->id) ? "reg" : "wire";
                break;
            case SIG_REGISTER:
                kw = "reg";
                break;
            case SIG_LATCH:
                kw = "reg";
                break;
            default:
                continue;
        }

        int width_for_decl = sig->width;
        if (sig->kind == SIG_NET && (width_for_decl <= 0) && mod->num_cdc_crossings > 0 && sig->name) {
            for (int c = 0; c < mod->num_cdc_crossings; ++c) {
                const IR_CDC *cdc = &mod->cdc_crossings[c];
                if (!cdc || !cdc->dest_alias_name) {
                    continue;
                }
                if (strcmp(cdc->dest_alias_name, sig->name) != 0) {
                    continue;
                }
                const IR_Signal *src_reg = find_signal_by_id(mod, cdc->source_reg_id);
                if (src_reg && src_reg->width > 0) {
                    width_for_decl = src_reg->width;
                    break;
                }
            }
        }

        /* Prevent yosys FSM extraction from re-encoding registers used
         * as SELECT selectors — the user's encoding is intentional and
         * the FSM optimizer can incorrectly discard reachable states. */
        if (sig->kind == SIG_REGISTER && signal_is_select_selector(mod, sig->id)) {
            fprintf(out, "    (* fsm_encoding = \"none\" *) %s", kw);
        } else if (sig->kind == SIG_LATCH && sig->iob) {
            fprintf(out, "    (* IOB = \"TRUE\" *) %s", kw);
        } else {
            fprintf(out, "    %s", kw);
        }
        emit_width_range(out, width_for_decl);
        char esc3[256];
        const char *sn3 = verilog_safe_name(sig->name ? sig->name : "jz_unnamed_signal", esc3, (int)sizeof(esc3));
        fprintf(out, " %s;\n", sn3);
    }

    fputc('\n', out);
}

/* -------------------------------------------------------------------------
 * Memory declaration emission
 * -------------------------------------------------------------------------
 */

void emit_memory_declarations(FILE *out, const IR_Module *mod)
{
    if (!out || !mod || mod->num_memories <= 0) {
        return;
    }

    fprintf(out, "    // Memories\n");

    for (int i = 0; i < mod->num_memories; ++i) {
        const IR_Memory *m = &mod->memories[i];
        const char *raw_name = (m->name && m->name[0] != '\0') ? m->name : "jz_mem";
        char mem_safe_buf[256];
        const char *name = verilog_memory_name(raw_name, mod->name, mem_safe_buf, sizeof(mem_safe_buf));
        bool needs_literal_init = m->init_kind == MEM_INIT_LITERAL &&
                                  m->init.literal.width > 0 &&
                                  m->depth > 0;
        char init_i_name[64];

        /* Declare implicit address registers for SYNC read ports.
         * Skip ports with synthetic addr register signals; those are
         * already declared as normal signals.
         */
        for (int p = 0; p < m->num_ports; ++p) {
            const IR_MemoryPort *mp = &m->ports[p];
            if (mp->kind != MEM_PORT_READ_SYNC || mp->addr_signal_id < 0) {
                continue;
            }
            if (mp->addr_reg_signal_id >= 0) {
                continue;
            }
            const char *port_name = (mp->name && mp->name[0] != '\0') ? mp->name : "rd";
            int addr_w = mp->address_width > 0 ? mp->address_width : m->address_width;
            fprintf(out, "    reg");
            emit_width_range(out, addr_w);
            fprintf(out, " %s_%s_addr;\n", name, port_name);
        }

        if (m->kind == MEM_KIND_BLOCK) {
            fprintf(out, "    (* ram_style = \"block\" *) reg");
        } else if (m->kind == MEM_KIND_DISTRIBUTED) {
            fprintf(out, "    (* ram_style = \"distributed\" *) reg");
        } else {
            fprintf(out, "    reg");
        }
        emit_width_range(out, m->word_width);
        if (m->depth > 0) {
            fprintf(out, " %s[0:%d];\n", name, m->depth - 1);
        } else {
            fprintf(out, " %s;\n", name);
        }

        if (needs_literal_init) {
            snprintf(init_i_name, sizeof(init_i_name), "jz_mem_init_i_%d", i);
            fprintf(out, "    integer %s;\n", init_i_name);
        }
    }

    fputc('\n', out);
}

/* -------------------------------------------------------------------------
 * Memory initialization emission
 * -------------------------------------------------------------------------
 */

static void mem_init_sanitize_fragment(const char *src,
                                       char *dst,
                                       size_t dst_size)
{
    if (!dst || dst_size == 0) {
        return;
    }
    if (!src || !src[0]) {
        snprintf(dst, dst_size, "jz");
        return;
    }

    size_t di = 0;
    for (size_t si = 0; src[si] && di + 1 < dst_size; ++si) {
        unsigned char ch = (unsigned char)src[si];
        if ((ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9')) {
            dst[di++] = (char)ch;
        } else {
            dst[di++] = '_';
        }
    }
    dst[di] = '\0';
}

static unsigned mem_init_word_bit(const uint8_t *word_bytes,
                                  int bytes_per_word,
                                  int word_width,
                                  int bit_from_msb)
{
    int target_bit = word_width - 1 - bit_from_msb;
    int byte_in_word = bytes_per_word - 1 - (target_bit / 8);
    int bit_in_byte = target_bit % 8;
    return (unsigned)((word_bytes[byte_in_word] >> bit_in_byte) & 1u);
}

static int mem_init_write_blob_sidecar_hex(const IR_Module *mod,
                                           const IR_Memory *mem,
                                           char *path_buf,
                                           size_t path_buf_size)
{
    if (!mod || !mem || !mem->init.blob || !path_buf || path_buf_size == 0) {
        return -1;
    }

    char mod_name[256];
    char mem_name[256];
    mem_init_sanitize_fragment(mod->name ? mod->name : "jz_module",
                               mod_name, sizeof(mod_name));
    mem_init_sanitize_fragment(mem->name ? mem->name : "jz_mem",
                               mem_name, sizeof(mem_name));

    int n = snprintf(path_buf, path_buf_size,
                     "jz_mem_init__%s__%s.hex",
                     mod_name, mem_name);
    if (n <= 0 || (size_t)n >= path_buf_size) {
        return -1;
    }

    FILE *fout = fopen(path_buf, "w");
    if (!fout) {
        return -1;
    }

    int bytes_per_word = (mem->word_width + 7) / 8;
    int hex_chars = (mem->word_width + 3) / 4;
    const uint8_t *bytes = mem->init.blob->bytes;

    for (int addr = 0; addr < mem->depth; ++addr) {
        const uint8_t *word = bytes + ((size_t)addr * (size_t)bytes_per_word);
        for (int nib = 0; nib < hex_chars; ++nib) {
            unsigned value = 0;
            for (int bit = 0; bit < 4; ++bit) {
                value <<= 1;
                int bit_from_msb = nib * 4 + bit;
                if (bit_from_msb < mem->word_width) {
                    value |= mem_init_word_bit(word, bytes_per_word,
                                               mem->word_width, bit_from_msb);
                }
            }
            fputc("0123456789ABCDEF"[value & 0xF], fout);
        }
        fputc('\n', fout);
    }

    if (fflush(fout) != 0 || ferror(fout)) {
        fclose(fout);
        return -1;
    }
    return fclose(fout) == 0 ? 0 : -1;
}

int emit_memory_initialization(FILE *out, const IR_Module *mod)
{
    if (!out || !mod || mod->num_memories <= 0) {
        return 0;
    }
    int errors = 0;

    for (int i = 0; i < mod->num_memories; ++i) {
        const IR_Memory *m = &mod->memories[i];
        const char *raw_name = (m->name && m->name[0] != '\0') ? m->name : "jz_mem";
        char mem_safe_buf[256];
        const char *name = verilog_memory_name(raw_name, mod->name, mem_safe_buf, sizeof(mem_safe_buf));

        if (m->init_kind == MEM_INIT_NONE) {
            continue;
        }

        if (m->init_kind == MEM_INIT_FILE) {
            fprintf(stderr,
                    "error: unresolved file-based memory init for memory %s in module %s\n",
                    name, mod->name ? mod->name : "?");
            errors++;
            continue;
        }

        if (m->init_kind == MEM_INIT_BLOB) {
            char hex_path[1024];
            if (mem_init_write_blob_sidecar_hex(mod, m,
                                                hex_path, sizeof(hex_path)) == 0) {
                fprintf(out, "    initial begin\n");
                fprintf(out, "        $readmemh(\"%s\", %s);\n",
                        hex_path, name);
                fprintf(out, "    end\n");
            } else {
                fprintf(stderr,
                        "error: failed to write memory init sidecar for memory %s in module %s\n",
                        name, mod->name ? mod->name : "?");
                errors++;
            }
            continue;
        }

        if (m->depth <= 0 || m->init_kind != MEM_INIT_LITERAL) {
            continue;
        }

        fprintf(out, "    initial begin\n");
        fprintf(out, "        for (jz_mem_init_i_%d = 0; jz_mem_init_i_%d < %d; jz_mem_init_i_%d = jz_mem_init_i_%d + 1) begin\n",
            i, i, m->depth, i, i);
        fprintf(out, "            %s[jz_mem_init_i_%d] = ", name, i);
        emit_literal(out, &m->init.literal);
        fprintf(out, ";\n");
        fprintf(out, "        end\n");
        fprintf(out, "    end\n");
    }

    fputc('\n', out);
    return errors ? -1 : 0;
}
